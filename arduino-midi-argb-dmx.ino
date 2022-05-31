/**
 * Title:       Arduino MIDI ARGB DMX
 * Created by:  Cooper Dalrymple
 * Date:        2022-05-06
 * License:     GNU GPL v3.0
 * Version:     1.0
 */

#define USB_MIDI        true
#define USB_TIMEOUT     2000

#if USB_MIDI
#include <USB-MIDI.h>
#else
#include <MIDI.h>
#endif

struct CustomMidiSettings : public MIDI_NAMESPACE::DefaultSettings {
    static const unsigned SysExMaxSize = 256;
    static const unsigned BaudRate = 31250;
};
#define MidiType MIDI_NAMESPACE::MidiType

#define SerialMidiMessage MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>, CustomMidiSettings>::MidiMessage
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial, CustomMidiSettings);
#if USB_MIDI
#define UsbMidiMessage MIDI_NAMESPACE::MidiInterface<USBMIDI_NAMESPACE::usbMidiTransport, CustomMidiSettings>::MidiMessage
USBMIDI_CREATE_CUSTOM_INSTANCE(0, MIDICoreUSB, CustomMidiSettings);
#endif

#include <DmxSimple.h>
#include <FastLED.h>

#include "color.h"
#include "palettes.h"
#include "preset.h"
#include "settings.h"

#define LED             13
#define LED_DURATION    10

#define ARGB_PIN        2
#define ARGB_ORDER      GRB
CRGB *argbLights;

#define DMX_PIN         7
#define DMX_ORDER       GBR
CRGB *dmxLights;

#define RGB_R           9
#define RGB_G           10
#define RGB_B           11
#define RGB_W           5

#define COLOR_UPDATE    10
#define PALETTE_UPDATE  1
#define PALETTE_CHANGES 48
#define PALETTE_ITERS   1

PresetData current_preset = default_preset;
CRGBPalette16 current_palette(CRGB::Black);
TBlendType blending = LINEARBLEND;

Settings settings;

#if USB_MIDI
void messageUartToUsb(SerialMidiMessage message);
void messageUsbToUart(UsbMidiMessage message);
#endif
void controlChange(byte channel, byte control, byte value);
void programChange(byte channel, byte program);

void setup() {

    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    #if USB_MIDI
    #else
    Serial.begin(9600);
    unsigned long usb_start = millis();
    while (!Serial && millis() - usb_start < USB_TIMEOUT) { }
    if (Serial) {
        Serial.println("Arduino MIDI ARGB DMX - Version 1.0 - 2022 Cooper Dalrymple");

        Serial.print("Midi Thru: ");
        if (settings.getMidiThru()) {
            Serial.print("On");
        } else {
            Serial.print("Off");
        }
        Serial.println();

        Serial.print("Midi Channel: ");
        if (settings.getMidiChannel() > 0) {
            Serial.print(settings.getMidiChannel());
        } else {
            Serial.print("Omni");
        }
        Serial.println();
    }
    #endif

    pinMode(0, INPUT);
    pinMode(1, OUTPUT);
    if (settings.getMidiThru()) {
        MIDICoreSerial.turnThruOn();
    } else {
        MIDICoreSerial.turnThruOff();
    }
    #if USB_MIDI
    MIDICoreSerial.setHandleMessage(messageUartToUsb);
    #endif
    MIDICoreSerial.setHandleControlChange(controlChange);
    MIDICoreSerial.setHandleProgramChange(programChange);
    MIDICoreSerial.begin(MIDI_CHANNEL_OMNI);

    #if USB_MIDI
    MIDICoreUSB.setHandleMessage(messageUsbToUart);
    MIDICoreUSB.setHandleControlChange(controlChange);
    MIDICoreUSB.setHandleProgramChange(programChange);
    MIDICoreUSB.begin(MIDI_CHANNEL_OMNI);
    #endif

    argbLights = new CRGB[settings.getArgbCount()];
    switch (settings.getArgbType()) {
        case FLTYPE_NEOPIXEL:
            FastLED.addLeds<NEOPIXEL, ARGB_PIN>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_SM16703:
            //FastLED.addLeds<SM16703, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_TM1829:
            //FastLED.addLeds<TM1829, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_TM1812:
            //FastLED.addLeds<TM1812, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_TM1809:
            FastLED.addLeds<TM1809, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_TM1804:
            FastLED.addLeds<TM1804, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_TM1803:
            FastLED.addLeds<TM1803, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_UCS1903:
            FastLED.addLeds<UCS1903, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_UCS1903B:
            //FastLED.addLeds<UCS1903B, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_UCS1904:
            //FastLED.addLeds<UCS1904, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_UCS2903:
            //FastLED.addLeds<UCS2903, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_WS2812:
            FastLED.addLeds<WS2812, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_WS2852:
            //FastLED.addLeds<WS2852, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_WS2812B:
            FastLED.addLeds<WS2812B, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_GS1903:
            //FastLED.addLeds<GS1903, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_SK6812:
            //FastLED.addLeds<SK6812, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_SK6822:
            //FastLED.addLeds<SK6822, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_APA106:
            //FastLED.addLeds<APA106, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_PL9823:
            //FastLED.addLeds<PL9823, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_WS2811:
            FastLED.addLeds<WS2811, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_WS2813:
            FastLED.addLeds<WS2813, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_APA104:
            //FastLED.addLeds<APA104, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_WS2811_400:
            //FastLED.addLeds<WS2811_400, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_GE8822:
            //FastLED.addLeds<GE8822, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_GW6205:
            FastLED.addLeds<GW6205, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_GW6205_400:
            //FastLED.addLeds<GW6205_400, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_LPD1886:
            //FastLED.addLeds<LPD1886, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_LPD1886_8BIT:
            //FastLED.addLeds<LPD1886_8BIT, ARGB_PIN, ARGB_ORDER>(argbLights, settings.getArgbCount()).setCorrection(TypicalLEDStrip);
            break;
        case FLTYPE_NONE:
        default:
            break;
    }

    dmxLights = new CRGB[settings.getDmxCount()];
    FastLED.addLeds<DMXSIMPLE, DMX_PIN, DMX_ORDER>(dmxLights, settings.getDmxCount());

    FastLED.setBrightness(settings.getBrightness());

    pinMode(RGB_R, OUTPUT);
    analogWrite(RGB_R, 0);
    pinMode(RGB_G, OUTPUT);
    analogWrite(RGB_G, 0);
    pinMode(RGB_B, OUTPUT);
    analogWrite(RGB_B, 0);
    pinMode(RGB_W, OUTPUT);
    analogWrite(RGB_W, 0);

    digitalWrite(LED, LOW); // Indicate that initialization is complete

}

CRGBPalette16 target_palette(CRGB::Black);
unsigned long palette_millis;
uint8_t palette_index = 0;
float palette_pos = 0;

unsigned long led_millis = -1;

#if USB_MIDI
void messageUartToUsb(SerialMidiMessage message) {
    UsbMidiMessage usbMessage;
    usbMessage.channel = message.channel;
    usbMessage.type = message.type;
    usbMessage.data1 = message.data1;
    usbMessage.data2 = message.data2;
    if (message.type == MidiType::SystemExclusive) {
        memcpy(usbMessage.sysexArray, message.sysexArray, message.sSysExMaxSize & sizeof(MIDI_NAMESPACE::DataByte));
    }
    usbMessage.valid = message.valid;
    usbMessage.length = message.length;
    MIDICoreUSB.send(usbMessage);
}
void messageUsbToUart(UsbMidiMessage message) {
    SerialMidiMessage serialMessage;
    serialMessage.channel = message.channel;
    serialMessage.type = message.type;
    serialMessage.data1 = message.data1;
    serialMessage.data2 = message.data2;
    if (message.type == MidiType::SystemExclusive) {
        memcpy(serialMessage.sysexArray, message.sysexArray, message.sSysExMaxSize & sizeof(MIDI_NAMESPACE::DataByte));
    }
    serialMessage.valid = message.valid;
    serialMessage.length = message.length;
    MIDICoreSerial.send(serialMessage);
}
#endif

void loop() {
    EVERY_N_MILLISECONDS(COLOR_UPDATE) {
        palette_pos += current_preset.palette_speed;
        if (palette_pos >= 256.0f) palette_pos -= 256.0f;
        palette_index = (uint8_t)palette_pos;

        switch (current_preset.mode) {
            default:
                FillLEDsFromPaletteColors(palette_index);
                break;
        }

        FastLED.show();
    }

    EVERY_N_MILLISECONDS(PALETTE_UPDATE) {
        #if PALETTE_ITERS > 1
        for (uint8_t i = 0; i < PALETTE_ITERS; i++) {
            nblendPaletteTowardPalette(current_palette, target_palette, PALETTE_CHANGES);
        }
        #else
        nblendPaletteTowardPalette(current_palette, target_palette, PALETTE_CHANGES);
        #endif
    }

    if (led_millis >= 0 && millis() - led_millis > LED_DURATION) {
        digitalWrite(LED, LOW);
        led_millis = -1;
    }

    MIDICoreSerial.read();
    #if USB_MIDI
    MIDICoreUSB.read();
    #endif
}

void triggerLed() {
    digitalWrite(LED, HIGH);
    led_millis = millis();
}

// LED Functions

uint8_t getW(CRGB color) {
    return min(color.r, min(color.g, color.b));
}

void FillLEDsFromPaletteColors(uint8_t index) {
    uint8_t brightness = 255;
    CRGB c = ColorFromPalette(current_palette, index, brightness, blending);

    analogWrite(RGB_R, c.r);
    analogWrite(RGB_G, c.g);
    analogWrite(RGB_B, c.b);
    analogWrite(RGB_W, getW(c));

    argbLights[0] = c;
    dmxLights[0] = c;
    for (int i = 1; i < max(settings.getArgbCount(), settings.getDmxCount()); i++) {
        index += 3;
        c = ColorFromPalette(current_palette, index, brightness, blending);
        if (i < settings.getArgbCount()) argbLights[i] = c;
        if (i < settings.getDmxCount()) dmxLights[i] = c;
    }
}

void updatePalette() {
    switch (current_preset.mode) {
        case ColorOff:
            target_palette = CRGBPalette16(CRGB::Black);
        case SolidRgb:
            target_palette = CRGBPalette16(current_preset.color_rgb);
            break;
        case SolidHsv:
            target_palette = CRGBPalette16(current_preset.color_hsv);
            break;
        case Palette:
            target_palette = *palettes[current_preset.palette];
            break;
    }
}

// Midi Events

bool updated = false;
void controlChange(byte channel, byte control, byte value) {
    if (settings.getMidiChannel() > 0 && channel != settings.getMidiChannel() - 1) return;

    updated = false;
    switch (control) {

        // Palette Controls (Effect Controller 1-2)
        case 12:
            current_preset.palette = map(value, 0, 127, 0, palette_count);
            if (current_preset.palette >= palette_count) current_preset.palette = palette_count - 1;
            updated = true;
            break;
        case 13:
            current_preset.palette_speed = (float)value / 32.0f;
            updated = true;
            break;

        // RGB Controls (General Purpose Controller 1-4)
        case 16:
            current_preset.color_rgb.r = value*2;
            updated = true;
            break;
        case 17:
            current_preset.color_rgb.g = value*2;
            updated = true;
            break;
        case 18:
            current_preset.color_rgb.b = value*2;
            updated = true;
            break;

        // HSV Controls (General Purpose Controller 5-8)
        case 80:
            current_preset.color_hsv.h = value*2;
            updated = true;
            break;
        case 81:
            current_preset.color_hsv.s = value*2;
            updated = true;
            break;
        case 82:
            current_preset.color_hsv.v = value*2;
            updated = true;
            break;

    }

    if (updated) {
        triggerLed();
        updatePalette();
    }
}

void programChange(byte channel, byte program) {
    if (settings.getMidiChannel() > 0 && channel != settings.getMidiChannel() - 1) return;

    if (program < COLOR_MODES) {
        current_preset.mode = (ColorMode)program;
        triggerLed();
        updatePalette();
    }
}
