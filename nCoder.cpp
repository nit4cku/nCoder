/*
 * Copyright (c) 2018 nitacku
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * @file        nCoder.cpp
 * @summary     Generic Encoder Library
 * @version     1.1
 * @author      nitacku
 * @data        15 July 2018
 */

#include "Arduino.h"
#include "nCoder.h"

static CNcoder* g_callback_object = nullptr; // Global pointer to class object

CNcoder::CNcoder(const uint8_t pin_button,
                 const CNcoder::ButtonMode button_mode,
                 const CNcoder::RotationMode rotation_mode)
    : m_pin_button{pin_button}
    , m_button_mode{button_mode}
    , m_rotation_mode{rotation_mode}
    , m_update{false}
    , m_state{VALUE_START}
    , m_rotation{Rotation::CW}
    , m_callback_function{nullptr}
{
	pinMode(m_pin_button, INPUT);
	pinMode(m_pin_A, INPUT);
	pinMode(m_pin_B, INPUT);

#ifdef ENABLE_PULLUPS
	digitalWrite(m_pin_button, HIGH);
	digitalWrite(m_pin_A, HIGH);
	digitalWrite(m_pin_B, HIGH);
#endif

    // Attach hardware interrupts
    attachInterrupt(0, InterruptEncoderWrapper, CHANGE);
    attachInterrupt(1, InterruptEncoderWrapper, CHANGE);
    
    g_callback_object = this; // Assign global object pointer
}


void CNcoder::SetCallback(void (*function_ptr)(void))
{
    m_callback_function = function_ptr;
}


void CNcoder::SetUpdate(void)
{
    m_update = true;
}


void CNcoder::SetRotation(const Rotation rotation)
{
    m_rotation = rotation;
}


bool CNcoder::IsUpdateAvailable(void)
{
    bool update = m_update;
    m_update = false; // Clear pending update
    return update;
}


CNcoder::Button CNcoder::GetButtonState(void)
{
    uint8_t value = digitalRead(m_pin_button);
    
    if (m_button_mode == CNcoder::ButtonMode::INVERTED)
    {
        value = !value;
    }
    
    return static_cast<CNcoder::Button>(value);
}


CNcoder::Rotation CNcoder::GetRotation(void)
{
    return m_rotation;
}


void CNcoder::InterruptEncoderWrapper(void)
{
    g_callback_object->InterruptEncoder();
}


void CNcoder::InterruptEncoder(void)
{
/*
 * The below state table has, for each state (row), the new state
 * to set based on the next encoder output. From left to right in,
 * the table, the encoder outputs are 00, 01, 10, 11, and the value
 * in that position is the new state to set.
 */

#ifdef HALF_STEP
    // Use the half-step state table (emits a code at 00 and 11)
    enum R : uint8_t
    {
        R_CCW_BEGIN     = 0x1,
        R_CW_BEGIN      = 0x2,
        R_START_M       = 0x3,
        R_CW_BEGIN_M    = 0x4,
        R_CCW_BEGIN_M   = 0x5,
    };
    
    static const unsigned char ttable[6][4] PROGMEM = {
        {R_START_M,                 R_CW_BEGIN,     R_CCW_BEGIN,    VALUE_START},
        // R_CCW_BEGIN
        {R_START_M | VALUE_DIR_CCW, VALUE_START,    R_CCW_BEGIN,    VALUE_START},
        // R_CW_BEGIN
        {R_START_M | VALUE_DIR_CW,  R_CW_BEGIN,     VALUE_START,    VALUE_START},
        // R_START_M (11)
        {R_START_M,                 R_CCW_BEGIN_M,  R_CW_BEGIN_M,   VALUE_START},
        // R_CW_BEGIN_M
        {R_START_M,                 R_START_M,      R_CW_BEGIN_M,   VALUE_START /*| VALUE_DIR_CW*/},
        // R_CCW_BEGIN_M
        {R_START_M,                 R_CCW_BEGIN_M,  R_START_M,      VALUE_START /*| VALUE_DIR_CCW*/},
    };
#else
    // Use the full-step state table (emits a code at 00 only)
    enum R : uint8_t
    {
        R_CW_FINAL      = 0x1,
        R_CW_BEGIN      = 0x2,
        R_CW_NEXT       = 0x3,
        R_CCW_BEGIN     = 0x4,
        R_CCW_FINAL     = 0x5,
        R_CCW_NEXT      = 0x6,
    };

    static const unsigned char ttable[7][4] PROGMEM = {
        // VALUE_START
        {VALUE_START,   R_CW_BEGIN,     R_CCW_BEGIN,    VALUE_START},
        // R_CW_FINAL
        {R_CW_NEXT,     VALUE_START,    R_CW_FINAL,     VALUE_START | VALUE_DIR_CW},
        // R_CW_BEGIN
        {R_CW_NEXT,     R_CW_BEGIN,     VALUE_START,    VALUE_START},
        // R_CW_NEXT
        {R_CW_NEXT,     R_CW_BEGIN,     R_CW_FINAL,     VALUE_START},
        // R_CCW_BEGIN
        {R_CCW_NEXT,    VALUE_START,    R_CCW_BEGIN,    VALUE_START},
        // R_CCW_FINAL
        {R_CCW_NEXT,    R_CCW_FINAL,    VALUE_START,    VALUE_START | VALUE_DIR_CCW},
        // R_CCW_NEXT
        {R_CCW_NEXT,    R_CCW_FINAL,    R_CCW_BEGIN,    VALUE_START},
    };
#endif
    
	// Grab state of input pins.
	uint8_t pinstate = (digitalRead(m_pin_B) << 1) | digitalRead(m_pin_A);
	// Determine new state from the pins and state table.
	m_state = pgm_read_byte_near(&ttable[m_state & 0x0F][pinstate]);
	// Mask emit bits, ie the generated event.
    uint8_t event = (m_state & 0x30);

    if (event)
    {
        if (m_rotation_mode == CNcoder::RotationMode::INVERTED)
        {
            event = ((event == VALUE_DIR_CW) ? VALUE_DIR_CCW : VALUE_DIR_CW); // Invert rotation
        }
        
        m_rotation = ((event == VALUE_DIR_CW) ? Rotation::CW : Rotation::CCW);
        SetUpdate(); // Set flag

        // Check that callback function exists
        if (m_callback_function != nullptr)
        {
            (*m_callback_function)(); // Call user function
        }
    }
}
