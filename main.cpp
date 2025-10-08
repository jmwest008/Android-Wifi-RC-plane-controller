#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"

namespace {
constexpr const char *kSsid = "PicoW-RCPlane";
constexpr const char *kPassword = "rcplane123";
constexpr uint16_t kUdpPort = 4444;
constexpr size_t kBufferSize = 16;
constexpr uint32_t kSafetyTimeoutMs = 1000;

constexpr std::array<uint, 4> kServoPins = {0, 1, 2, 3};
constexpr uint32_t kPwmWrap = 20000;           // 20 ms period -> 50 Hz
constexpr float kPwmClockDiv = 125.0f;         // 125 MHz / 125 = 1 MHz -> 1us resolution
constexpr uint16_t kServoNeutralUs = 1500;
constexpr uint16_t kServoRangeUs = 500;
constexpr uint16_t kEscMinUs = 1000;
constexpr uint16_t kEscRangeUs = 1000;

struct FlightPacket {
    float roll;
    float pitch;
    float yaw;
    float throttle_norm;
};

struct ControlState {
    absolute_time_t last_packet;
    bool controls_active;
};

float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void configure_pwm_pin(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);

    pwm_set_clkdiv(slice, kPwmClockDiv);
    pwm_set_wrap(slice, kPwmWrap);
    pwm_set_chan_level(slice, channel, kServoNeutralUs);
    pwm_set_enabled(slice, true);
}

void set_servo_pulse(uint pin, uint16_t microseconds) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice, channel, microseconds);
}

std::array<uint16_t, 4> controls_to_servo(const FlightPacket &packet) {
    float roll = clamp(packet.roll, -1.0f, 1.0f);
    float pitch = clamp(packet.pitch, -1.0f, 1.0f);
    float yaw = clamp(packet.yaw, -1.0f, 1.0f);
    float throttle = clamp(packet.throttle_norm, 0.0f, 1.0f);

    uint16_t roll_servo = static_cast<uint16_t>(kServoNeutralUs + roll * kServoRangeUs);
    uint16_t pitch_servo = static_cast<uint16_t>(kServoNeutralUs + pitch * kServoRangeUs);
    uint16_t yaw_servo = static_cast<uint16_t>(kServoNeutralUs + yaw * kServoRangeUs);
    uint16_t throttle_esc = static_cast<uint16_t>(kEscMinUs + throttle * kEscRangeUs);

    return {roll_servo, pitch_servo, yaw_servo, throttle_esc};
}

bool parse_packet(const std::array<uint8_t, kBufferSize> &buffer, FlightPacket *out_packet) {
    static_assert(sizeof(FlightPacket) == kBufferSize, "Unexpected packet size");

    FlightPacket packet;
    std::memcpy(&packet, buffer.data(), sizeof(FlightPacket));

    if (std::isnan(packet.roll) || std::isnan(packet.pitch) || std::isnan(packet.yaw) ||
        std::isnan(packet.throttle_norm)) {
        return false;
    }

    *out_packet = packet;
    return true;
}

void set_safe_mode() {
    for (size_t i = 0; i < kServoPins.size(); ++i) {
        uint16_t pulse = (i == kServoPins.size() - 1) ? kEscMinUs : kServoNeutralUs;
        set_servo_pulse(kServoPins[i], pulse);
    }
}

void handle_udp_packet(ControlState *state, pbuf *packet_buffer) {
    if (!packet_buffer || packet_buffer->tot_len != kBufferSize) {
        return;
    }

    std::array<uint8_t, kBufferSize> buffer{};
    pbuf_copy_partial(packet_buffer, buffer.data(), buffer.size(), 0);

    FlightPacket packet;
    if (!parse_packet(buffer, &packet)) {
        return;
    }

    auto servo_pulses = controls_to_servo(packet);
    for (size_t i = 0; i < kServoPins.size(); ++i) {
        set_servo_pulse(kServoPins[i], servo_pulses[i]);
    }

    state->last_packet = get_absolute_time();
    state->controls_active = true;

    printf("Controls: R:%.2f P:%.2f Y:%.2f T:%d\n", packet.roll, packet.pitch, packet.yaw,
           static_cast<int>(servo_pulses[3]));
}

void udp_receive_callback(void *arg, udp_pcb *pcb, pbuf *packet_buffer, const ip_addr_t *addr,
                          u16_t port) {
    (void)pcb;
    (void)addr;
    (void)port;

    auto *state = static_cast<ControlState *>(arg);
    handle_udp_packet(state, packet_buffer);

    if (packet_buffer) {
        pbuf_free(packet_buffer);
    }
}

}  // namespace

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Failed to initialise CYW43\n");
        return 1;
    }

    cyw43_arch_enable_ap_mode(kSsid, kPassword, CYW43_AUTH_WPA2_AES_PSK);
    printf("Access Point active: %s\n", kSsid);

    for (uint pin : kServoPins) {
        configure_pwm_pin(pin);
    }

    set_safe_mode();

    ControlState state{
        .last_packet = get_absolute_time(),
        .controls_active = false,
    };

    udp_pcb *pcb = udp_new();
    if (!pcb) {
        printf("Failed to allocate UDP control block\n");
        cyw43_arch_deinit();
        return 1;
    }

    err_t err = udp_bind(pcb, IP_ADDR_ANY, kUdpPort);
    if (err != ERR_OK) {
        printf("Failed to bind UDP PCB: %d\n", static_cast<int>(err));
        udp_remove(pcb);
        cyw43_arch_deinit();
        return 1;
    }

    udp_recv(pcb, udp_receive_callback, &state);

    printf("UDP server listening on port %u\n", kUdpPort);

    while (true) {
        absolute_time_t now = get_absolute_time();
        if (state.controls_active &&
            absolute_time_diff_us(state.last_packet, now) > kSafetyTimeoutMs * 1000) {
            set_safe_mode();
            state.controls_active = false;
        }

        cyw43_arch_poll();
        sleep_ms(10);
    }

    udp_remove(pcb);
    cyw43_arch_deinit();
    return 0;
}
