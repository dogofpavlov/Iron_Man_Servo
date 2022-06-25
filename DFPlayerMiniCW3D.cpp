/*!
 * @file DFPlayerMiniCW3D.cpp
 * @brief DFPlayer - An Arduino Mini MP3 Player From DFRobot
 * @n Header file for DFRobot's DFPlayer
 *
 * @copyright	[DFRobot]( http://www.dfrobot.com ), 2016
 * @copyright	GNU Lesser General Public License
 *
 * @author [Angelo](Angelo.qiao@dfrobot.com)
 * @version  V1.0.3
 * @date  2016-12-07
 */

#include "DFPlayerMiniCW3D.h"

void DFPlayerMiniCW3D::setTimeOut(unsigned long timeOutDuration){
  _timeOutDuration = timeOutDuration;
}

void DFPlayerMiniCW3D::uint16ToArray(uint16_t value, uint8_t *array){
  *array = (uint8_t)(value>>8);
  *(array+1) = (uint8_t)(value);
}

uint16_t DFPlayerMiniCW3D::calculateCheckSum(uint8_t *buffer){
  uint16_t sum = 0;
  for (int i=Stack_Version; i<Stack_CheckSum; i++) {
    sum += buffer[i];
  }
  return -sum;
}

void DFPlayerMiniCW3D::sendStack(){
  if (_sending[Stack_ACK]) {  //if the ack mode is on wait until the last transmition
    while (_isSending) {
      delay(0);
      available();
    }
  }

#ifdef _DEBUG
  Serial.println();
  Serial.print(F("sending:"));
  for (int i=0; i<DFPLAYER_SEND_LENGTH; i++) {
    Serial.print(_sending[i],HEX);
    Serial.print(F(" "));
  }
  Serial.println();
#endif
  _serial->write(_sending, DFPLAYER_SEND_LENGTH);
  _timeOutTimer = millis();
  _isSending = _sending[Stack_ACK];
  
  if (!_sending[Stack_ACK]) { //if the ack mode is off wait 10 ms after one transmition.
    delay(10);
  }
}

void DFPlayerMiniCW3D::sendStack(uint8_t command){
  sendStack(command, 0);
}

void DFPlayerMiniCW3D::sendStack(uint8_t command, uint16_t argument){
  _sending[Stack_Command] = command;
  uint16ToArray(argument, _sending+Stack_Parameter);
  uint16ToArray(calculateCheckSum(_sending), _sending+Stack_CheckSum);
  sendStack();
}

void DFPlayerMiniCW3D::sendStack(uint8_t command, uint8_t argumentHigh, uint8_t argumentLow){
  uint16_t buffer = argumentHigh;
  buffer <<= 8;
  sendStack(command, buffer | argumentLow);
}

void DFPlayerMiniCW3D::enableACK(){
  _sending[Stack_ACK] = 0x01;
}

void DFPlayerMiniCW3D::disableACK(){
  _sending[Stack_ACK] = 0x00;
}

bool DFPlayerMiniCW3D::waitAvailable(unsigned long duration){
  unsigned long timer = millis();
  if (!duration) {
    duration = _timeOutDuration;
  }
  while (!available()){
    if (millis() - timer > duration) {
      return false;
    }
    delay(0);
  }
  return true;
}

bool DFPlayerMiniCW3D::begin(Stream &stream, bool isACK, bool doReset){
  _serial = &stream;
  
  if (isACK) {
    enableACK();
  }
  else{
    disableACK();
  }
  
  if (doReset) {
    reset();
    waitAvailable(2000);
    delay(200);
  }
  else {
    // assume same state as with reset(): online
    _handleType = DFPlayerCardOnline;
  }

  return (readType() == DFPlayerCardOnline) || (readType() == DFPlayerUSBOnline) || !isACK;
}

uint8_t DFPlayerMiniCW3D::readType(){
  _isAvailable = false;
  return _handleType;
}

uint16_t DFPlayerMiniCW3D::read(){
  _isAvailable = false;
  return _handleParameter;
}

bool DFPlayerMiniCW3D::handleMessage(uint8_t type, uint16_t parameter){
  _receivedIndex = 0;
  _handleType = type;
  _handleParameter = parameter;
  _isAvailable = true;
  return _isAvailable;
}

bool DFPlayerMiniCW3D::handleError(uint8_t type, uint16_t parameter){
  handleMessage(type, parameter);
  _isSending = false;
  return false;
}

uint8_t DFPlayerMiniCW3D::readCommand(){
  _isAvailable = false;
  return _handleCommand;
}

void DFPlayerMiniCW3D::parseStack(){
  uint8_t handleCommand = *(_received + Stack_Command);
  if (handleCommand == 0x41) { //handle the 0x41 ack feedback as a spcecial case, in case the pollusion of _handleCommand, _handleParameter, and _handleType.
    _isSending = false;
    return;
  }
  
  _handleCommand = handleCommand;
  _handleParameter =  arrayToUint16(_received + Stack_Parameter);

  switch (_handleCommand) {
    case 0x3D:
      handleMessage(DFPlayerPlayFinished, _handleParameter);
      break;
    case 0x3F:
      if (_handleParameter & 0x01) {
        handleMessage(DFPlayerUSBOnline, _handleParameter);
      }
      else if (_handleParameter & 0x02) {
        handleMessage(DFPlayerCardOnline, _handleParameter);
      }
      else if (_handleParameter & 0x03) {
        handleMessage(DFPlayerCardUSBOnline, _handleParameter);
      }
      break;
    case 0x3A:
      if (_handleParameter & 0x01) {
        handleMessage(DFPlayerUSBInserted, _handleParameter);
      }
      else if (_handleParameter & 0x02) {
        handleMessage(DFPlayerCardInserted, _handleParameter);
      }
      break;
    case 0x3B:
      if (_handleParameter & 0x01) {
        handleMessage(DFPlayerUSBRemoved, _handleParameter);
      }
      else if (_handleParameter & 0x02) {
        handleMessage(DFPlayerCardRemoved, _handleParameter);
      }
      break;
    case 0x40:
      handleMessage(DFPlayerError, _handleParameter);
      break;
    case 0x3C:
    case 0x3E:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
      handleMessage(DFPlayerFeedBack, _handleParameter);
      break;
    default:
      handleError(WrongStack);
      break;
  }
}

uint16_t DFPlayerMiniCW3D::arrayToUint16(uint8_t *array){
  uint16_t value = *array;
  value <<=8;
  value += *(array+1);
  return value;
}

bool DFPlayerMiniCW3D::validateStack(){
  return calculateCheckSum(_received) == arrayToUint16(_received+Stack_CheckSum);
}

bool DFPlayerMiniCW3D::available(){
  while (_serial->available()) {
    delay(0);
    if (_receivedIndex == 0) {
      _received[Stack_Header] = _serial->read();
#ifdef _DEBUG
      Serial.print(F("received:"));
      Serial.print(_received[_receivedIndex],HEX);
      Serial.print(F(" "));
#endif
      if (_received[Stack_Header] == 0x7E) {
        _receivedIndex ++;
      }
    }
    else{
      _received[_receivedIndex] = _serial->read();
#ifdef _DEBUG
      Serial.print(_received[_receivedIndex],HEX);
      Serial.print(F(" "));
#endif
      switch (_receivedIndex) {
        case Stack_Version:
          if (_received[_receivedIndex] != 0xFF) {
            return handleError(WrongStack);
          }
          break;
        case Stack_Length:
          if (_received[_receivedIndex] != 0x06) {
            return handleError(WrongStack);
          }
          break;
        case Stack_End:
#ifdef _DEBUG
          Serial.println();
#endif
          if (_received[_receivedIndex] != 0xEF) {
            return handleError(WrongStack);
          }
          else{
            if (validateStack()) {
              _receivedIndex = 0;
              parseStack();
              return _isAvailable;
            }
            else{
              return handleError(WrongStack);
            }
          }
          break;
        default:
          break;
      }
      _receivedIndex++;
    }
  }
  
  if (_isSending && (millis()-_timeOutTimer>=_timeOutDuration)) {
    return handleError(TimeOut);
  }
  
  return _isAvailable;
}

#ifndef LITE
void DFPlayerMiniCW3D::next(){
  sendStack(0x01);
}

void DFPlayerMiniCW3D::previous(){
  sendStack(0x02);
}
#endif

void DFPlayerMiniCW3D::play(int fileNumber){
  sendStack(0x03, fileNumber);
}

#ifndef LITE
void DFPlayerMiniCW3D::volumeUp(){
  sendStack(0x04);
}

void DFPlayerMiniCW3D::volumeDown(){
  sendStack(0x05);
}
#endif

void DFPlayerMiniCW3D::volume(uint8_t volume){
  sendStack(0x06, volume);
}

void DFPlayerMiniCW3D::EQ(uint8_t eq) {
  sendStack(0x07, eq);
}

#ifndef LITE
void DFPlayerMiniCW3D::loop(int fileNumber) {
  sendStack(0x08, fileNumber);
}
#endif

void DFPlayerMiniCW3D::outputDevice(uint8_t device) {
  sendStack(0x09, device);
  delay(200);
}

#ifndef LITE
void DFPlayerMiniCW3D::sleep(){
  sendStack(0x0A);
}
#endif

void DFPlayerMiniCW3D::reset(){
  sendStack(0x0C);
}

#ifndef LITE
void DFPlayerMiniCW3D::start(){
  sendStack(0x0D);
}

void DFPlayerMiniCW3D::pause(){
  sendStack(0x0E);
}

void DFPlayerMiniCW3D::playFolder(uint8_t folderNumber, uint8_t fileNumber){
  sendStack(0x0F, folderNumber, fileNumber);
}

void DFPlayerMiniCW3D::outputSetting(bool enable, uint8_t gain){
  sendStack(0x10, enable, gain);
}

void DFPlayerMiniCW3D::enableLoopAll(){
  sendStack(0x11, 0x01);
}

void DFPlayerMiniCW3D::disableLoopAll(){
  sendStack(0x11, 0x00);
}

void DFPlayerMiniCW3D::playMp3Folder(int fileNumber){
  sendStack(0x12, fileNumber);
}

void DFPlayerMiniCW3D::advertise(int fileNumber){
  sendStack(0x13, fileNumber);
}

void DFPlayerMiniCW3D::playLargeFolder(uint8_t folderNumber, uint16_t fileNumber){
  sendStack(0x14, (((uint16_t)folderNumber) << 12) | fileNumber);
}

void DFPlayerMiniCW3D::stopAdvertise(){
  sendStack(0x15);
}

void DFPlayerMiniCW3D::stop(){
  sendStack(0x16);
}

void DFPlayerMiniCW3D::loopFolder(int folderNumber){
  sendStack(0x17, folderNumber);
}

void DFPlayerMiniCW3D::randomAll(){
  sendStack(0x18);
}

void DFPlayerMiniCW3D::enableLoop(){
  sendStack(0x19, 0x00);
}

void DFPlayerMiniCW3D::disableLoop(){
  sendStack(0x19, 0x01);
}

void DFPlayerMiniCW3D::enableDAC(){
  sendStack(0x1A, 0x00);
}

void DFPlayerMiniCW3D::disableDAC(){
  sendStack(0x1A, 0x01);
}
#endif

int DFPlayerMiniCW3D::readState(){
  sendStack(0x42);
  if (waitAvailable()) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int DFPlayerMiniCW3D::readVolume(){
  sendStack(0x43);
  if (waitAvailable()) {
    return read();
  }
  else{
    return -1;
  }
}

#ifndef LITE
int DFPlayerMiniCW3D::readEQ(){
  sendStack(0x44);
  if (waitAvailable()) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int DFPlayerMiniCW3D::readFileCounts(uint8_t device){
  switch (device) {
    case DFPLAYER_DEVICE_U_DISK:
      sendStack(0x47);
      break;
    case DFPLAYER_DEVICE_SD:
      sendStack(0x48);
      break;
    case DFPLAYER_DEVICE_FLASH:
      sendStack(0x49);
      break;
    default:
      break;
  }
  
  if (waitAvailable()) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int DFPlayerMiniCW3D::readCurrentFileNumber(uint8_t device){
  switch (device) {
    case DFPLAYER_DEVICE_U_DISK:
      sendStack(0x4B);
      break;
    case DFPLAYER_DEVICE_SD:
      sendStack(0x4C);
      break;
    case DFPLAYER_DEVICE_FLASH:
      sendStack(0x4D);
      break;
    default:
      break;
  }
  if (waitAvailable()) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int DFPlayerMiniCW3D::readFileCountsInFolder(int folderNumber){
  sendStack(0x4E, folderNumber);
  if (waitAvailable()) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int DFPlayerMiniCW3D::readFolderCounts(){
  sendStack(0x4F);
  if (waitAvailable()) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int DFPlayerMiniCW3D::readFileCounts(){
  return readFileCounts(DFPLAYER_DEVICE_SD);
}

int DFPlayerMiniCW3D::readCurrentFileNumber(){
  return readCurrentFileNumber(DFPLAYER_DEVICE_SD);
}
#endif

