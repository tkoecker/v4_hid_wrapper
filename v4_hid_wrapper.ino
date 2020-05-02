#include <hidboot.h>
#include <usbhub.h>
#include <Wire.h>

#include <SPI.h>
#if ARDUINO > 10605
#include <Keyboard.h>
#include <Mouse.h>
#endif

#define JUMPER_MODE      2   // mode jumper pin for single device (switches between keyboard or mouse mode)
#define DUAL_MODE_PIN    20  // pin for selecting two device mode (one pro micro acts as mouse, one as keyboard)
#define MASTER_PIN       19  // pin for determining master in two device mode (master reads from the host shield, sends mouse events via usb, and sends keyboard events via i2c to 2nd pro micro)

#define MODIFIER_OFFSET  128 // Keyboard library maps modifiers to 128+<number>
#define KEYCODE_OFFSET   136 // Keyboard library maps USB HID usage codes to 136+<usage code>

bool dual_mode = false;
bool master = false;

struct MOUSEINFO_with_wheel {
  struct MOUSEINFO mi;
  int8_t dWheel;
};

class MouseRptParser : public MouseReportParser {
  public:
    void Parse(USBHID *hid __attribute__((unused)), bool is_rpt_id __attribute__((unused)), uint8_t len __attribute__((unused)), uint8_t *buf) override {
      MOUSEINFO_with_wheel *pmi = (MOUSEINFO_with_wheel*)buf;
      if (len > 3) {
        OnMouseWheel(pmi);
      }
      MouseReportParser::Parse(hid, is_rpt_id, len, buf);
    }
  protected:
    void OnMouseWheel(MOUSEINFO_with_wheel *mi) {
      Mouse.move(0, 0, mi->dWheel);
    }
    void OnMouseMove(MOUSEINFO *mi) {
      Mouse.move(mi->dX, mi->dY);
    }
    void OnLeftButtonUp(MOUSEINFO *mi) {
      Mouse.release(MOUSE_LEFT);
    }
    void OnLeftButtonDown(MOUSEINFO *mi) {
      Mouse.press(MOUSE_LEFT);
    }
    void OnRightButtonUp(MOUSEINFO *mi) {
      Mouse.release(MOUSE_RIGHT);
    }
    void OnRightButtonDown(MOUSEINFO *mi) {
      Mouse.press(MOUSE_RIGHT);
    }
    void OnMiddleButtonUp(MOUSEINFO *mi) {
      Mouse.release(MOUSE_MIDDLE);
    }
    void OnMiddleButtonDown(MOUSEINFO *mi) {
      Mouse.press(MOUSE_MIDDLE);
    }
};

class KbdRptParser : public KeyboardReportParser {
  protected:
    void OnControlKeysChanged(uint8_t before, uint8_t after) {
      for (int i = 0; i < 8; i++, before >>= 1, after >>= 1) {
        if ((before & 0x01) == 0 && (after & 0x01) == 1) {
          if (dual_mode) sendEvent(0, MODIFIER_OFFSET + i);
          else Keyboard.press(MODIFIER_OFFSET + i);
        } else if ((before & 0x01) == 1 && (after & 0x01) == 0) {
          if (dual_mode) sendEvent(1, MODIFIER_OFFSET + i);
          else Keyboard.release(MODIFIER_OFFSET + i);
        }
      }
    }
    void OnKeyDown(uint8_t mod, uint8_t key) {
      if (dual_mode) sendEvent(0, KEYCODE_OFFSET + key);
      else Keyboard.press(KEYCODE_OFFSET + key);
    }
    void OnKeyUp(uint8_t mod, uint8_t key) {
      if (dual_mode) sendEvent(1, KEYCODE_OFFSET + key);
      else Keyboard.release(KEYCODE_OFFSET + key);
    }
};

USB     Usb;
USBHub  Hub(&Usb);

HIDBoot<USB_HID_PROTOCOL_KEYBOARD | USB_HID_PROTOCOL_MOUSE> HidComposite(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE> HidMouse(&Usb);

KbdRptParser KbdPrs;
MouseRptParser MousePrs;

void sendEvent(uint8_t type, uint8_t value) {
  Wire.beginTransmission(9);
  Wire.write(type);
  Wire.write(value);
  Wire.endTransmission();
}

void receiveEvent(int bytes) {
  uint8_t type = Wire.read();
  uint8_t value = Wire.read();
  switch (type) {
    case 0:
      Keyboard.press(value);
      break;
    case 1:
      Keyboard.release(value);
      break;
  }
}

void setup() {
  pinMode(DUAL_MODE_PIN, INPUT_PULLUP);
  pinMode(MASTER_PIN, INPUT_PULLUP);
  dual_mode = !digitalRead(20);
  master = !digitalRead(19);
  if (dual_mode) {
    if (master) {
      Wire.begin();
    } else {
      Wire.begin(9);
      Wire.onReceive(receiveEvent);
      return;
    }
  }

  if (Usb.Init() == -1)
    ; // error

  delay(200);

  HidComposite.SetReportParser(0, &KbdPrs);
  HidComposite.SetReportParser(1, &MousePrs);
  HidKeyboard.SetReportParser(0, &KbdPrs);
  HidMouse.SetReportParser(0, &MousePrs);
}

void loop() {
  if (!dual_mode || master) {
    Usb.Task();
  }
}

// Needed for patched USBCore that either provides a mouse or a keyboard hid device.
int HID_GetDeviceType() {
  pinMode(DUAL_MODE_PIN, INPUT_PULLUP);
  pinMode(MASTER_PIN, INPUT_PULLUP);
  pinMode(JUMPER_MODE, INPUT_PULLUP);
  bool dual_mode = !digitalRead(20);
  bool master = !digitalRead(19);
  int type;
  if (dual_mode) {
    type = master ? 2 : 1;
  } else {
    // jumper on input pin 2 (pull-up -> connect to gnd) to select between keyboard and mouse mode
    // default to mouse if the jumper is not present
    type = digitalRead(JUMPER_MODE) ? 2 : 1;
  }
  return type; 
}

