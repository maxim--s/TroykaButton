#include "TroykaButton.h"

TroykaButton::TroykaButton(uint8_t pin, uint32_t timeHold, bool pullUP) {
    _pin = pin;
    _timeHold = timeHold;
    _pullUP = pullUP;
}

// инициализация кнопки
void TroykaButton::begin() {
    pinMode(_pin, INPUT);
    _msButtonState = 0;
}

// считывание состояние кнопки
void TroykaButton::read() {
    _stateButton = false;
    bool buttonStateNow = !digitalRead(_pin);
    if (!_pullUP) {
        buttonStateNow = !buttonStateNow;
    }
    if (buttonStateNow && buttonStateNow != _buttonStateWas && millis() - _msButtonState > DEBOUNCE_TIME) {
        _buttonStateNowLong = false;
        _msButtonState = millis();
        _stateButton = ON_PRESS;
    }
    if (!buttonStateNow && buttonStateNow != _buttonStateWas &&  !_buttonStateNowLong && _timeHold && millis() - _msButtonState > DEBOUNCE_TIME) {
        _msButtonState = millis();
        _stateButton = ON_RELEASE;
    }
    if (buttonStateNow && !_buttonStateNowLong && millis() - _msButtonState > _timeHold) {
        _buttonStateNowLong = true;
        _msButtonState = millis();
        _stateButton = ON_PRESS_LONG;
    }
    _buttonStateWas = buttonStateNow;
}

// определение клика кнопки
bool TroykaButton::justPressed() const {
    return _stateButton == ON_PRESS ? 1 : 0;
}

// определение отжатие кнопки
bool TroykaButton::justReleased() const {
    return _stateButton == ON_RELEASE ? 1 : 0;
}

// определение зажатие кнопки (по умолчанию 2 секунды)
bool TroykaButton::isHold() const {
    return _stateButton == ON_PRESS_LONG ? 1 : 0;
}
