/****************************************************************************/
//  Function:       Header file for TroykaButton
//  Arduino IDE:    Arduino-1.8.13
//  Author:         Igor Dementiev and Maxim Shatskih
//  Date:           Aug 14,2020
//  Version:        v2.0
//  by www.amperka.ru and contributors
/****************************************************************************/

#ifndef _TROYKA_BUTTON_H_
#define _TROYKA_BUTTON_H_

#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

//
//  ВНИМАНИЕ:
//
// - если кнопка была аппаратно нажата в момент выполнения конструктора или reinit(),
//     например, если Arduino включили с нажатой кнопкой,
//    то произойдет то же самое, как если бы кнопка была отпущена,
//    а нажали ее сразу после возврата из конструктора или reinit()
// - эта логика необходима для реализации подавления дребезга,
//    с учетом того, что в конструкторе, как правило, невозможно прочесть состояние пина
//

//
//              Логика подавления дребезга
//
// - ожидается, пока электрическое состояние пина не менялось в течение DEBOUNCE_TIME
// - после этого состояние пина считается окончательным
//

// время подавления дребезга в ms
#define DEBOUNCE_TIME   50

class TroykaButton
{
public: // API
  // pin - номер пина
  // timeHold - время (в ms) непрерывного удержания кнопки,
  //  после которого нажатие понимается как долгое удержание
  // pullUP - должен быть true, если пин оттянут резистором к питанию,
  //  и false - если к земле
  TroykaButton(uint8_t pin, uint32_t timeHold = 2000, bool pullUP = true);
  // полный сброс объекта, после которого он становится таким же,
  //  как сразу после конструктора + begin() + единственного read()
  // повторно звать begin() - не требуется
  // новый таймаут от setTimeHold() НЕ сбрасывается к старому
  // может быть полезен в случае, если на кнопку временно повесили прерывание,
  //  а затем вернулись к использованию этого класса
  inline void reinit() __attribute__((always_inline));
  // изменяет заданное в конструкторе время (в ms) непрерывного удержания кнопки,
  //  после которого нажатие понимается как долгое удержание
  // может быть полезен в случае, если используется логика нажатий на кнопку
  //  азбукой Морзе (или как-то вроде), как в некоторых брелках автосигнализаций
  inline void setTimeHold(uint32_t newTimeHold) __attribute__((always_inline));
  // инициализация кнопки (для вызова в setup())
  inline void begin() __attribute__((always_inline));
  // считывание данных с кнопки
  // обновляет внутреннее состояние объекта на основании состояния пина
  // для вызова в loop() каждый раз
  // метод неблокирующий (внутри нет delay() и аналогов)
  void read();
  // определение клика кнопки
  // возвращает true тогда и только тогда, когда:
  //  1) ранее отпущенная кнопка была нажата
  //  И
  //  2) только при первом вызове после того, как выполнилось условие 1)
  // возвращает false во всех остальных случаях
  // можно сказать, что этот метод работает "по фронту"
  // метод неблокирующий (внутри нет delay() и аналогов)
  inline bool justPressed() __attribute__((always_inline));
  // определение отжатия кнопки
  // то же, что и justPressed(), но условие 1) выглядит как:
  //  "ранее нажатая кнопка была отпущена"
  inline bool justReleased() __attribute__((always_inline));
  // определение удержания кнопки
  // то же, что и justPressed(), но условие 1) выглядит как:
  //  "нажатая кнопка удерживалась нажатой в течение >= timeHold миллисекунд"
  inline bool justHeld() __attribute__((always_inline));
  // true, если кнопка нажата "здесь и сейчас", иначе false
  // метод неблокирующий (внутри нет delay() и аналогов)
  inline bool isPressed() const __attribute__((always_inline));
  // true, если кнопка отпущена "здесь и сейчас", иначе false
  // метод неблокирующий (внутри нет delay() и аналогов)
  inline bool isReleased() const __attribute__((always_inline));
  // true, если кнопка нажата "здесь и сейчас",
  //  и удерживалась нажатой в течение >= timeHold миллисекунд, иначе false
  // метод неблокирующий (внутри нет delay() и аналогов)
  inline bool isHold() const __attribute__((always_inline));
  // определение короткого клика, если сработал метод isHold() клик не сработает.
  // то же, что и justReleased() (именно так! для совместимости со старым кодом)
  //  с той лишь разницей, что justReleased() вернет true
  //  и в случае долгого (timeHold) удержания кнопки,
  //  а этот метод в таком случае вернет false
  // isClick() не до конца совместим с justReleased(),
  //   а именно, если isClick() уже вернул true, то justReleased() вернет false
  //   а если justReleased() уже вернул true, то isClick() вернет false
  // возможно совместное использование isClick() и justReleased() в виде:
  //   - первым в loop() должен зваться isClick()
  //   - justReleased() должен зваться, только если isClick() вернул false
  //   - тогда, если justReleased() вернет true,
  //       это будет означать "отпускание после долгого удержания"
  //   - результат isClick() при этом можно проигнорировать
  bool isClick();
private: // данные инициализации
  // номер пина
  const uint8_t _pin;
  // время длительного зажатия кнопки
  uint32_t _timeHold;
  // выбор подтяжки
  const bool _pullUP;
private: // данные состояния
  enum _State {
    stReleased,
    stPressed,
    stLongHold, // во всех современных компиляторах можно ставить здесь запятую
  };
  // запомненное электрическое состояние пина (с поправкой на _pullUP)
  bool _pinState;
  // millis(), когла было прочитано и сохранено _pinState
  unsigned long _tmPinState;
  // true, если течет временной интервал подавления дребезга, иначе false
  bool _isDebounceTimeoutActive;
  // текущее "софтовое" состояние кнопки
  _State _state;
  // предыдущее состояние кнопки (используется только в isClick())
  _State _prevState;
  // true, если _state изменилось, и после этого еще не был вызван
  //  один из justXxx() методов или isClick()
  bool _isStateDirty;
  // millis() последнего обновления _state
  // нужно для таймаута удержания, мы не можем использовать _tmPinState
  //  для него, ибо тогда ложное кратковременное дребезжащее размыкание нажатой кнопки
  //  сбросит логику timeHold
  unsigned long _tmState;
private: // внутренности
  // внутренний движок под конструктором и reinit()
  // инициализирует все поля состояния
  void _resetObject();
  // единый, для экономии места под код, внутренний движок под всеми justXxx()
  bool _justXxxInner(_State stateToTest);
  // читает электрическое состояние пина (с поправкой на _pullUP)
  inline bool _readPinState() const __attribute__((always_inline));
  // запоминает состояние пина правильным образом
  inline void _updatePinState(bool newPinState) __attribute__((always_inline));
  // oбновляет _state правильным образом
  inline void _updateState(_State newState) __attribute__((always_inline));
};

// Тела публичных инлайнов

// они настолько крошечны
//  (а приватные из них редко зовутся по количеству мест в коде),
//  что есть реальные основания полагать,
//  что они меньше прологов-эпилогов функций
//   (а некоторые ненамного длиннее опкода call)
// потому, для экономии места под код - инлайны
// естественно, производительность тоже повышается

void TroykaButton::reinit() {
  _resetObject();
  begin(); // мало ли что успели понаделать с этим пином
  read(); // для ре-инициализации подавления дребезга
}

void TroykaButton::setTimeHold(uint32_t newTimeHold) { _timeHold = newTimeHold; }

void TroykaButton::begin() { pinMode(_pin, INPUT); }

bool TroykaButton::justPressed() { return _justXxxInner(stPressed); }

bool TroykaButton::justReleased() { return _justXxxInner(stReleased); }

bool TroykaButton::justHeld() { return _justXxxInner(stLongHold); }

bool TroykaButton::isPressed() const { return _state == stPressed || _state == stLongHold; }

bool TroykaButton::isReleased() const { return _state == stReleased; }

bool TroykaButton::isHold() const { return _state == stLongHold; }

#endif // _TROYKA_BUTTON_H_
