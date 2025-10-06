#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"

#include "lwip/errno.h"
#include "lwip/fcntl.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

namespace {
constexpr const char *kSsid = "PicoW-RCPlane";
constexpr const char *kPassword = "rcplane123";
constexpr uint16_t kUdpPort = 4444;
constexpr size_t kBufferSize = 16;
constexpr uint32_t kSafetyTimeoutMs = 1000;

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

    int sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("Failed to create UDP socket: %d\n", errno);
        return 1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
    addr.sin_port = PP_HTONS(kUdpPort);

    if (lwip_bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        printf("Failed to bind UDP socket: %d\n", errno);
        lwip_close(sock);
        return 1;
    }

    int flags = lwip_fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        lwip_fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    printf("UDP server listening on port %u\n", kUdpPort);
    absolute_time_t last_packet = get_absolute_time();

    while (true) {
        std::array<uint8_t, kBufferSize> buffer{};
        struct sockaddr_in source = {};
        socklen_t source_len = sizeof(source);

        int received = lwip_recvfrom(sock, buffer.data(), buffer.size(), 0,
                                     reinterpret_cast<struct sockaddr *>(&source), &source_len);

        if (received == static_cast<int>(kBufferSize)) {
            FlightPacket packet;
            if (parse_packet(buffer, &packet)) {
                last_packet = get_absolute_time();
                auto servo_pulses = controls_to_servo(packet);

                for (size_t i = 0; i < kServoPins.size(); ++i) {
                    set_servo_pulse(kServoPins[i], servo_pulses[i]);
                }

                printf("Controls: R:%.2f P:%.2f Y:%.2f T:%d\n", packet.roll, packet.pitch, packet.yaw,
                       static_cast<int>(servo_pulses[3]));
            }
        } else if (received < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                printf("Socket error: %d\n", errno);
            }
        }

        if (absolute_time_diff_us(last_packet, get_absolute_time()) > kSafetyTimeoutMs * 1000) {
            set_safe_mode();
        }

        cyw43_arch_poll();
        sleep_ms(10);
    }

    cyw43_arch_deinit();
    return 0;
}
