/*
*  2017 Reaper7
*/

#include <Esp.h>
#include "ESP8266SPIFFSUpdateFirmware.h"
#include <string.h>

SPIFFSUpdateFirmwareClass::SPIFFSUpdateFirmwareClass()
: _firmwareext(FIRMWAREEXT)
, _firmwarepath(FIRMWAREPATH)
, _filesystemexists(false)
, _minSketchSpace(MINBINSIZE)
, _maxSketchSpace(MINBINSIZE)
, _start_callback(NULL)
, _end_callback(NULL)
//, _error_callback(NULL)
, _progress_callback(NULL)
{
}

SPIFFSUpdateFirmwareClass::~SPIFFSUpdateFirmwareClass() {
  FirmwareListClean();
}

void SPIFFSUpdateFirmwareClass::onStart(THandlerFunction fn) {
    _start_callback = fn;
}

void SPIFFSUpdateFirmwareClass::onEnd(THandlerFunction fn) {
    _end_callback = fn;
}

void SPIFFSUpdateFirmwareClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
}
/*
void SPIFFSUpdateFirmwareClass::onError(THandlerFunction_Error fn) {
    _error_callback = fn;
}
*/

uint32_t SPIFFSUpdateFirmwareClass::getSize(uint8_t _index) {
  if (_filesystemexists && _index >= 0 && _index < FirmwareList.size()) {
    FirmwareFileList_t entry = FirmwareList[_index];
    return (atoi(entry.fmsize));
  } else
    return (0);
}

String SPIFFSUpdateFirmwareClass::getName(uint8_t _index) {
  if (_filesystemexists && _index >= 0 && _index < FirmwareList.size()) {
    FirmwareFileList_t entry = FirmwareList[_index];
    return ((String)entry.fmname);
  } else
    return ("");
}

bool SPIFFSUpdateFirmwareClass::startUpdate(String _fn, bool _rst) {
  bool ret = false;
  String _fname = _firmwarepath + "/" + _fn + _firmwareext;

  if ((_filesystemexists) && (_fname != "")) {
    if (SPIFFS.exists(_fname)) {
      File firmfile = SPIFFS.open(_fname, "r");
      uint32_t fsize = firmfile.size();
      if (Update.begin(_maxSketchSpace, U_FLASH)) {
        if (_start_callback) {
          _start_callback();
        }
        uint32_t filerest = 0;
        if (_progress_callback) {
          _progress_callback(filerest, fsize);
        }
        while (firmfile.available()) {
          uint8_t ibuffer[128];
          firmfile.read((uint8_t *)ibuffer, 128);
          filerest += Update.write(ibuffer, sizeof(ibuffer));
          if (_progress_callback) {
            _progress_callback(filerest, fsize);
          }           
        }
        if (Update.end(true)) {
          ret = true;
          if (_end_callback) {
            _end_callback();
          }
        }
      }
      firmfile.close();
      if (ret && _rst) {
        delay(100);
        ESP.restart();
      }
    }
  }
  return ret;
}

bool SPIFFSUpdateFirmwareClass::startUpdate(uint8_t _index, bool _rst) {
  if (_filesystemexists && _index >= 0 && _index < FirmwareList.size())
    return (startUpdate(getName(_index), _rst));
  else
    return false;
}

uint8_t SPIFFSUpdateFirmwareClass::getCount() {
  if (_filesystemexists) {
    Dir dir = SPIFFS.openDir(_firmwarepath);
    while (dir.next()) {
      String _fname = dir.fileName();
      if (_fname.endsWith(_firmwareext)) {
        uint32_t _fsize = dir.fileSize();
        if (_fsize > _minSketchSpace && _fsize <= _maxSketchSpace) {
          _fname = _fname.substring(_firmwarepath.length() + 1, _fname.length() - _firmwareext.length());
          FirmwareListAdd(_fname.c_str(), _fsize);
        }
      }
    }
  }
  return (FirmwareList.size());
}

void SPIFFSUpdateFirmwareClass::FirmwareListAdd(const char* fmname, uint32_t fmsize) {
  FirmwareFileList_t newFirmware;
  char _fsch[16];
  sprintf(_fsch,"%lu", fmsize);  
  newFirmware.fmsize = strdup(_fsch);
  newFirmware.fmname = strdup(fmname);
  FirmwareList.push_back(newFirmware);
}

void SPIFFSUpdateFirmwareClass::FirmwareListClean(void) {
  //clear & free firmware list
  for(uint8_t i = 0; i < FirmwareList.size(); i++) {
    FirmwareFileList_t entry = FirmwareList[i];
    if(entry.fmname) {
      free(entry.fmname);
    }
    if(entry.fmsize) {
      free(entry.fmsize);
    }
  }
  FirmwareList.clear();
}

bool SPIFFSUpdateFirmwareClass::begin(String _fwpath) {
  _filesystemexists = SPIFFS.begin();
  if (_filesystemexists) {
    _firmwarepath = _fwpath;
    _maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000; 
  }
  return (_filesystemexists);
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SPIFFSFIRMWARE)
SPIFFSUpdateFirmwareClass SPIFFSFirmware = SPIFFSUpdateFirmwareClass();
#endif
