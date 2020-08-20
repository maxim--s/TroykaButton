/****************************************************************************/
//  Function:       Cpp file for TroykaButton
//  Arduino IDE:    Arduino-1.8.13
//  Author:         Igor Dementiev and Maxim Shatskih
//  Date:           Aug 14,2020
//  Version:        v2.0
//  by www.amperka.ru and contributors
/****************************************************************************/

#include "TroykaButton.h"

// Тела приватных инлайнов

// != для типа bool есть xor
bool TroykaButton::_readPinState() const { return (digitalRead(_pin) == HIGH) != _pullUP; }

void TroykaButton::_updatePinState(bool newPinState) {
  _pinState = newPinState;
  _tmPinState = millis();
}

void TroykaButton::_updateState(_State newState) {
  _prevState = _state;
  _state = newState;
  _isStateDirty = true;
  _tmState = millis();
}

// API

// pin - номер пина
// timeHold - время (в ms) непрерывного удержания кнопки,
//  после которого нажатие понимается как долгое удержание
// pullUP - должен быть true, если пин оттянут резистором к питанию,
//  и false - если к земле
TroykaButton::TroykaButton(uint8_t pin, uint32_t timeHold, bool pullUP) :
    _pin(pin),
    _timeHold(timeHold),
    _pullUP(pullUP) {
  _resetObject();
}

// считывание данных с кнопки
// обновляет внутреннее состояние объекта на основании состояния пина
// для вызова в loop() каждый раз
// метод неблокирующий (внутри нет delay() и аналогов)
void TroykaButton::read() {
  // для начала всегда отработаем длинное удержание,
  //  даже если активно подавление дребезга
  //  (для этого не нужно щупать пин)
  // если после подавления дребезга кнопка окажется отпущенной -
  //  состояние обновится еще раз ниже
  if (_state == stPressed && millis() - _tmState >= _timeHold)
    // после этого _state более не stPressed, второй раз код не выполнится
    _updateState(stLongHold);
  // щупаем пин
  bool currentPinState = _readPinState();
  if (currentPinState != _pinState) {
    // пин изменился
    // (ре)стартуем таймаут подавления дребезга
    //  с новым (возможно, шумовым и не окончательным) состоянием пина
    _isDebounceTimeoutActive = true;
    _updatePinState(currentPinState);
  } else {
    // пин не изменился
    if (_isDebounceTimeoutActive && millis() - _tmPinState >= DEBOUNCE_TIME) {
      // подавление дребезга активно, а пин не менялся в течение всего таймаута
      //  (если бы он менялся, код выше рестартнул бы таймаут)
      // _pinState теперь суть окончательное состояние пина
      // подавление дребезга завершилось
      _isDebounceTimeoutActive = false;
      // делаем реальную работу
      _State newState = _pinState ? stPressed : stReleased;
      // switch() ниже нужен для следующего сценария:
      //  - если контакты удерживаемой кнопки кратковременно разомкнулись
      //      (из-за низкого качества кнопки, окисления и др)
      //  - в этом случае дважды стартанет подавление дребезга,
      //      и, после прекращения ложного размыкания
      //          (и истечения DEBOUNCE_TIME после того)
      //      мы попадем сюда с newState == stPressed и _state == stPressed или stLongHold
      //  - если убрать switch(),
      //      то в вышеуказанном сценарии _updateState() взведет _isStateDirty,
      //      а следом justPressed() представит ложное размыкание как второе нажатие
      //          (без промежуточного отпускания)
      //          БАГ
      //  - кроме того, _updateState() рестартнет _tmState,
      //      и ложное размыкание рестартнет таймаут timeHold
      //          ВТОРОЙ БАГ
      //  - примерно то же самое произойдет и в случае (хотя и малореальном),
      //      если отпущенная кнопка кратковременно замкнулась
      //      (из-за краткого КЗ в проводах на макетке и др)
      //  - а потому - switch(), избавляющий от _updateState()
      //      (взвода _isStateDirty и рестарта _tmState)
      //      в этих сценариях
      //  - если вкратце: switch() отрабатывает сценарий,
      //     когда пин подребезжал-подребезжал, и вернулся в старое состояние
      switch (_state) {
      case stReleased:
        if (newState == stReleased)
          // больше нечего делать
          return;
        break;
      case stPressed:
      case stLongHold:
        if (newState == stPressed)
          return;
        break;
      }
      // и вот теперь наконец обновляем "софтовое" состояние кнопки
      _updateState(newState);
    }
    //else
    //  - read() ничего не делает вне таймаута подавления дребезга (или пока он не истек),
    //     если пин не менялся
  }
}

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
bool TroykaButton::isClick() {
  if (_state == stReleased && _isStateDirty && _prevState == stPressed) {
    // мы прочитали вновь установленное состояние
    _isStateDirty = false;
    return true;
  }
  return false;
}

// внутренности

// внутренний движок под конструктором и reinit()
// инициализирует все поля состояния
void TroykaButton::_resetObject() {
  // при инициализации считаем, что пин не взведен
  // если же он на самом деле взведен - то первый же read() в loop() или в reinit()
  //  начнет таймаут подавления дребезга,
  //  который в итоге (если кнопка действительно нажата) приведет к тому же,
  //  что и нажатие кнопки сразу по возврату из конструктора или reinit()
  _pinState = false;
  // поскольку мы устанавливаем _isDebounceTimeoutActive в false,
  //  по факту безразлично, как инициализировать это поле
  _tmPinState = 0;
  // таймаут активируется первый раз при первом же чтении взведенного пина в read()
  _isDebounceTimeoutActive = false;
  // при инициализации считаем, что кнопка не нажата
  // далее см. комментарий выше к _pinState
  _state = stReleased;
  // поскольку мы устанавливаем _isStateDirty в false,
  //  по факту безразлично, как инициализировать это поле, нужное только для isClick()
  _prevState = stReleased;
  _isStateDirty = false;
  // поскольку мы устанавливаем _state в stReleased,
  //  по факту безразлично, как инициализировать это поле
  _tmState = 0;
}

// единый, для экономии места под код, внутренний движок под всеми justXxx()
bool TroykaButton::_justXxxInner(_State stateToTest) {
  if (_state == stateToTest && _isStateDirty) {
    // мы прочитали вновь установленное состояние
    _isStateDirty = false;
    return true;
  }
  return false;
}
