package com.rcplane.controller

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.os.Bundle
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import com.rcplane.controller.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    
    private lateinit var binding: ActivityMainBinding
    private val udpService = UdpService()
    private var isArmed = false
    
    // Current flight values
    private var roll = 0f
    private var pitch = 0f  
    private var yaw = 0f
    private var throttle = 0
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        setupJoysticks()
        setupThrottleSlider()
        setupArmButtons()
        startConnectionMonitoring()
        
        // Try to connect to Pico W
        connectToPicoW()
    }
    
    private fun setupJoysticks() {
        // Left joystick for pitch and roll
        binding.leftJoystick.onJoystickMoveListener = { x, y ->
            roll = x
            pitch = y
            udpService.updateFlightData(roll = roll, pitch = pitch)
            updateDataDisplay()
        }
        
        // Right joystick for yaw (only X-axis used)
        binding.rightJoystick.onJoystickMoveListener = { x, _ ->
            yaw = x
            udpService.updateFlightData(yaw = yaw)
            updateDataDisplay()
        }
    }
    
    private fun setupThrottleSlider() {
        binding.throttleSlider.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                throttle = progress
                binding.throttleValue.text = "${progress}%"
                udpService.updateFlightData(throttle = throttle)
                updateDataDisplay()
            }
            
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }
    
    private fun setupArmButtons() {
        binding.armButton.setOnClickListener {
            isArmed = true
            udpService.updateFlightData(armed = true)
            updateArmButtons()
        }
        
        binding.disarmButton.setOnClickListener {
            isArmed = false
            udpService.updateFlightData(armed = false)
            updateArmButtons()
            // Reset throttle when disarming
            binding.throttleSlider.progress = 0
        }
        
        updateArmButtons()
    }
    
    private fun updateArmButtons() {
        if (isArmed) {
            binding.armButton.backgroundTintList = ContextCompat.getColorStateList(this, R.color.armed_color)
            binding.disarmButton.backgroundTintList = ContextCompat.getColorStateList(this, R.color.disarmed_color)
        } else {
            binding.armButton.backgroundTintList = ContextCompat.getColorStateList(this, R.color.disarmed_color)
            binding.disarmButton.backgroundTintList = ContextCompat.getColorStateList(this, R.color.armed_color)
        }
    }
    
    private fun updateDataDisplay() {
        val formattedData = String.format(
            "Roll: %.2f | Pitch: %.2f | Yaw: %.2f | Throttle: %d | Armed: %s",
            roll, pitch, yaw, throttle, if (isArmed) "YES" else "NO"
        )
        binding.dataDisplay.text = formattedData
    }
    
    private fun connectToPicoW() {
        lifecycleScope.launch {
            val connected = udpService.startService("192.168.4.1") // Default Pico W AP IP
            updateConnectionStatus(connected)
        }
    }
    
    private fun startConnectionMonitoring() {
        lifecycleScope.launch {
            while (true) {
                val connected = udpService.isConnected() && isWifiConnected()
                updateConnectionStatus(connected)
                delay(1000) // Check every second
            }
        }
    }
    
    private fun isWifiConnected(): Boolean {
        val connectivityManager = getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        val network = connectivityManager.activeNetwork ?: return false
        val networkCapabilities = connectivityManager.getNetworkCapabilities(network) ?: return false
        return networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)
    }
    
    private fun updateConnectionStatus(connected: Boolean) {
        runOnUiThread {
            binding.statusText.text = if (connected) {
                getString(R.string.connected)
            } else {
                getString(R.string.disconnected)
            }
            binding.statusText.setTextColor(
                ContextCompat.getColor(
                    this,
                    if (connected) R.color.disarmed_color else R.color.armed_color
                )
            )
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        udpService.stopService()
    }
    
    override fun onPause() {
        super.onPause()
        // Disarm and stop throttle when app goes to background for safety
        isArmed = false
        binding.throttleSlider.progress = 0
        udpService.updateFlightData(armed = false, throttle = 0)
        updateArmButtons()
    }
}