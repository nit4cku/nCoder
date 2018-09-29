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
 * @file        nCoder.h
 * @summary     Generic Encoder Library
 * @version     1.1
 * @author      nitacku
 * @data        15 July 2018
 */

#ifndef _ENCODER_H
#define _ENCODER_H

#include "Arduino.h"

// Enable this to emit codes twice per step.
#define HALF_STEP

// Enable weak pullups
//#define ENABLE_PULLUPS

class CNcoder
{
    private:
    
    enum value_t : uint8_t
    {
        VALUE_START   = 0x0,
        VALUE_DIR_NONE  = 0x0,  // No complete step yet
        VALUE_DIR_CW    = 0x20, // Clockwise step
        VALUE_DIR_CCW   = 0x10, // Counter-clockwise step
    };
    
    public:

	enum class Button : uint8_t
	{
		DOWN,
		UP,
	};

    enum class Rotation : uint8_t
    {
        CW,
        CCW,
    };
    
    enum class ButtonMode : uint8_t
    {
        NORMAL,
        INVERTED,
    };
    
    enum class RotationMode : uint8_t
    {
        NORMAL,
        INVERTED,
    };

	CNcoder(const uint8_t pin_button, const ButtonMode button_mode, const RotationMode rotation_mode);

    // Update functions
    void SetCallback(void (*function_ptr)(void));
    void SetUpdate(void);
    void SetRotation(const Rotation rotation);

    // State functions
    bool IsUpdateAvailable(void);
    Button GetButtonState(void);
    Rotation GetRotation(void);

    private:
    const uint8_t m_pin_A = 3;
    const uint8_t m_pin_B = 2;
    const uint8_t m_pin_button;
    
    ButtonMode m_button_mode;
    RotationMode m_rotation_mode;
    bool m_update;
	uint8_t m_state;
    Rotation m_rotation;

    void (*m_callback_function)();
    void InterruptEncoder(void);
    static void InterruptEncoderWrapper(void);
};

#endif
