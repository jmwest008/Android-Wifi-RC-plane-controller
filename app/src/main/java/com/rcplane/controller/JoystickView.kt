package com.rcplane.controller

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import kotlin.math.*

class JoystickView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var backgroundPaint = Paint().apply {
        color = Color.argb(64, 0, 0, 0)
        style = Paint.Style.FILL
        isAntiAlias = true
    }

    private var knobPaint = Paint().apply {
        color = Color.argb(255, 76, 175, 80)
        style = Paint.Style.FILL
        isAntiAlias = true
    }

    private var centerX = 0f
    private var centerY = 0f
    private var radius = 0f
    private var knobRadius = 0f
    private var knobX = 0f
    private var knobY = 0f

    var onJoystickMoveListener: ((x: Float, y: Float) -> Unit)? = null

    // Normalized values from -1.0 to 1.0
    var xValue = 0f
        private set
    var yValue = 0f
        private set

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        centerX = w / 2f
        centerY = h / 2f
        radius = min(w, h) / 2f * 0.8f
        knobRadius = radius * 0.25f
        resetKnob()
    }

    private fun resetKnob() {
        knobX = centerX
        knobY = centerY
        xValue = 0f
        yValue = 0f
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        // Draw background circle
        canvas.drawCircle(centerX, centerY, radius, backgroundPaint)

        // Draw center cross lines
        val crossPaint = Paint().apply {
            color = Color.argb(128, 255, 255, 255)
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }
        canvas.drawLine(centerX - radius, centerY, centerX + radius, centerY, crossPaint)
        canvas.drawLine(centerX, centerY - radius, centerX, centerY + radius, crossPaint)

        // Draw knob
        canvas.drawCircle(knobX, knobY, knobRadius, knobPaint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                val dx = event.x - centerX
                val dy = event.y - centerY
                val distance = sqrt(dx * dx + dy * dy)

                if (distance <= radius) {
                    knobX = event.x
                    knobY = event.y
                } else {
                    val angle = atan2(dy, dx)
                    knobX = centerX + cos(angle) * radius
                    knobY = centerY + sin(angle) * radius
                }

                // Calculate normalized values
                xValue = (knobX - centerX) / radius
                yValue = -(knobY - centerY) / radius // Invert Y axis

                onJoystickMoveListener?.invoke(xValue, yValue)
                invalidate()
                return true
            }
            MotionEvent.ACTION_UP -> {
                resetKnob()
                onJoystickMoveListener?.invoke(xValue, yValue)
                invalidate()
                return true
            }
        }
        return super.onTouchEvent(event)
    }
}