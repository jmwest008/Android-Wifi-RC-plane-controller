package com.rcplane.controller

import kotlinx.coroutines.*
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder

class UdpService {
    private var socket: DatagramSocket? = null
    private var job: Job? = null
    private var isRunning = false
    
    // Pico W default AP settings
    private var targetAddress: InetAddress? = null
    private val targetPort = 4444
    private val sendRate = 20L // 50Hz (20ms interval)
    
    // Flight control data
    data class FlightData(
        val roll: Float = 0f,      // -1.0 to 1.0
        val pitch: Float = 0f,     // -1.0 to 1.0  
        val yaw: Float = 0f,       // -1.0 to 1.0
        val throttle: Int = 0,     // 0 to 100
        val armed: Boolean = false
    )
    
    private var currentData = FlightData()
    
    fun updateFlightData(
        roll: Float? = null,
        pitch: Float? = null, 
        yaw: Float? = null,
        throttle: Int? = null,
        armed: Boolean? = null
    ) {
        currentData = currentData.copy(
            roll = roll ?: currentData.roll,
            pitch = pitch ?: currentData.pitch,
            yaw = yaw ?: currentData.yaw,
            throttle = throttle ?: currentData.throttle,
            armed = armed ?: currentData.armed
        )
    }
    
    fun startService(ipAddress: String = "192.168.4.1"): Boolean {
        return try {
            if (isRunning) {
                stopService()
            }
            
            targetAddress = InetAddress.getByName(ipAddress)
            socket = DatagramSocket()
            
            job = CoroutineScope(Dispatchers.IO).launch {
                isRunning = true
                while (isActive && isRunning) {
                    try {
                        sendFlightData()
                        delay(sendRate)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
            true
        } catch (e: Exception) {
            e.printStackTrace()
            false
        }
    }
    
    fun stopService() {
        isRunning = false
        job?.cancel()
        socket?.close()
        socket = null
        targetAddress = null
    }
    
    private suspend fun sendFlightData() {
        val targetAddr = targetAddress ?: return
        val sock = socket ?: return
        
        // Create binary packet: 4 floats + 1 int + 1 boolean = 21 bytes
        val buffer = ByteBuffer.allocate(21).apply {
            order(ByteOrder.LITTLE_ENDIAN)
            putFloat(currentData.roll)
            putFloat(currentData.pitch) 
            putFloat(currentData.yaw)
            putFloat(currentData.throttle / 100f) // Normalize to 0-1
            putInt(if (currentData.armed) 1 else 0)
            put(if (currentData.armed) 1.toByte() else 0.toByte())
        }
        
        val packet = DatagramPacket(
            buffer.array(),
            buffer.array().size,
            targetAddr,
            targetPort
        )
        
        withContext(Dispatchers.IO) {
            sock.send(packet)
        }
    }
    
    fun isConnected(): Boolean = isRunning && socket != null && !socket!!.isClosed
}
