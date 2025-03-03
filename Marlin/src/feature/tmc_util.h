/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../inc/MarlinConfig.h"
#if HAS_TRINAMIC
  #include <TMCStepper.h>
#endif

#define TMC_X_LABEL 'X', '0'
#define TMC_Y_LABEL 'Y', '0'
#define TMC_Z_LABEL 'Z', '0'

#define TMC_X2_LABEL 'X', '2'
#define TMC_Y2_LABEL 'Y', '2'
#define TMC_Z2_LABEL 'Z', '2'
#define TMC_Z3_LABEL 'Z', '3'

#define TMC_E0_LABEL 'E', '0'
#define TMC_E1_LABEL 'E', '1'
#define TMC_E2_LABEL 'E', '2'
#define TMC_E3_LABEL 'E', '3'
#define TMC_E4_LABEL 'E', '4'
#define TMC_E5_LABEL 'E', '5'

template<char AXIS_LETTER, char DRIVER_ID>
class TMCStorage {
  protected:
    // Only a child class has access to constructor => Don't create on its own! "Poor man's abstract class"
    TMCStorage() {}

    uint16_t val_mA = 0;

  public:
    #if ENABLED(MONITOR_DRIVER_STATUS)
      uint8_t otpw_count = 0;
      bool flag_otpw = false;
      bool getOTPW() { return flag_otpw; }
      void clear_otpw() { flag_otpw = 0; }
    #endif

    uint16_t getMilliamps() { return val_mA; }

    void printLabel() {
      SERIAL_CHAR(AXIS_LETTER);
      if (DRIVER_ID > '0') SERIAL_CHAR(DRIVER_ID);
    }
};

template<class TMC, char AXIS_LETTER, char DRIVER_ID>
class TMCMarlin : public TMC, public TMCStorage<AXIS_LETTER, DRIVER_ID> {
  public:
    TMCMarlin(uint16_t cs_pin, float RS) :
      TMC(cs_pin, RS)
      {}
    TMCMarlin(uint16_t CS, float RS, uint16_t pinMOSI, uint16_t pinMISO, uint16_t pinSCK) :
      TMC(CS, RS, pinMOSI, pinMISO, pinSCK)
      {}
    uint16_t rms_current() { return TMC::rms_current(); }
    void rms_current(uint16_t mA) {
      this->val_mA = mA;
      TMC::rms_current(mA);
    }
    void rms_current(uint16_t mA, float mult) {
      this->val_mA = mA;
      TMC::rms_current(mA, mult);
    }
};
template<char AXIS_LETTER, char DRIVER_ID>
class TMCMarlin<TMC2208Stepper, AXIS_LETTER, DRIVER_ID> : public TMC2208Stepper, public TMCStorage<AXIS_LETTER, DRIVER_ID> {
  public:
    TMCMarlin(Stream * SerialPort, float RS, bool has_rx=true) :
      TMC2208Stepper(SerialPort, RS, has_rx=true)
      {}
    TMCMarlin(uint16_t RX, uint16_t TX, float RS, bool has_rx=true) :
      TMC2208Stepper(RX, TX, RS, has_rx=true)
      {}
    uint16_t rms_current() { return TMC2208Stepper::rms_current(); }
    void rms_current(uint16_t mA) {
      this->val_mA = mA;
      TMC2208Stepper::rms_current(mA);
    }
    void rms_current(uint16_t mA, float mult) {
      this->val_mA = mA;
      TMC2208Stepper::rms_current(mA, mult);
    }
};
template<char AXIS_LETTER, char DRIVER_ID>
class TMCMarlin<TMC2130Stepper, AXIS_LETTER, DRIVER_ID> : public TMC2130Stepper, public TMCStorage<AXIS_LETTER, DRIVER_ID> {
  protected:
  #if USE_SENSORLESS
    uint16_t val_mA_homing = 0;
    bool is_sensorless_homing = false;
  #endif // USE_SENSORLESS
  public:
    TMCMarlin(uint16_t cs_pin, float RS) :
      TMC2130Stepper(cs_pin, RS)
      {}
    TMCMarlin(uint16_t CS, float RS, uint16_t pinMOSI, uint16_t pinMISO, uint16_t pinSCK) :
      TMC2130Stepper(CS, RS, pinMOSI, pinMISO, pinSCK)
      {}
    uint16_t rms_current() { return this->val_mA; }
    void rms_current(uint16_t mA) {
      this->val_mA = mA;
      TMC2130Stepper::rms_current(mA);
    }
    void rms_current(uint16_t mA, float mult) {
      this->val_mA = mA;
      TMC2130Stepper::rms_current(mA, mult);
    }
    #if USE_SENSORLESS
    uint16_t rms_current_homing() {
      return this->val_mA_homing;
    }
    void rms_current_homing(uint16_t mA) {
      this->val_mA_homing = mA;
      if (this->is_sensorless_homing) {
        TMC2130Stepper::rms_current(mA);
      }
    }
    void set_sensorless_homing(bool enable) {
      if (this->is_sensorless_homing != enable) {
        this->is_sensorless_homing = enable;
        TMC2130Stepper::rms_current(enable ? this->val_mA_homing : this->val_mA);
        this->TCOOLTHRS(enable ? 0xFFFFF : 0);
        #if ENABLED(STEALTHCHOP)
          this->en_pwm_mode(!enable);
        #endif
        this->diag1_stall(enable ? 1 : 0);
      }
    }
    #endif // USE_SENSORLESS
};

constexpr uint32_t _tmc_thrs(const uint16_t msteps, const int32_t thrs, const uint32_t spmm) {
  return 12650000UL * msteps / (256 * thrs * spmm);
}

template<typename TMC>
void tmc_get_current(TMC &st) {
  st.printLabel();
  SERIAL_ECHOLNPAIR(" driver current: ", st.getMilliamps());
}
template<typename TMC>
void tmc_set_current(TMC &st, const int mA) {
  st.rms_current(mA);
}
#if ENABLED(MONITOR_DRIVER_STATUS)
  template<typename TMC>
  void tmc_report_otpw(TMC &st) {
    st.printLabel();
    SERIAL_ECHOPGM(" temperature prewarn triggered: ");
    serialprintPGM(st.getOTPW() ? PSTR("true") : PSTR("false"));
    SERIAL_EOL();
  }
  template<typename TMC>
  void tmc_clear_otpw(TMC &st) {
    st.clear_otpw();
    st.printLabel();
    SERIAL_ECHOLNPGM(" prewarn flag cleared");
  }
#endif
template<typename TMC>
void tmc_get_pwmthrs(TMC &st, const uint16_t spmm) {
  st.printLabel();
  SERIAL_ECHOLNPAIR(" stealthChop max speed: ", _tmc_thrs(st.microsteps(), st.TPWMTHRS(), spmm));
}
template<typename TMC>
void tmc_set_pwmthrs(TMC &st, const int32_t thrs, const uint32_t spmm) {
  st.TPWMTHRS(_tmc_thrs(st.microsteps(), thrs, spmm));
}
template<typename TMC>
void tmc_get_sgt(TMC &st) {
  st.printLabel();
  SERIAL_ECHOPGM(" homing sensitivity: ");
  SERIAL_PRINTLN(st.sgt(), DEC);
}
template<typename TMC>
void tmc_set_sgt(TMC &st, const int8_t sgt_val) {
  st.sgt(sgt_val);
}
template<typename TMC>
void tmc_get_homing_current(TMC &st) {
  st.printLabel();
  SERIAL_ECHOLNPAIR(" sensorless homing driver current: ", st.rms_current_homing());
}
template<typename TMC>
void tmc_set_homing_current(TMC &st, const int mA) {
  st.rms_current_homing(mA);
}

void monitor_tmc_driver();

#if ENABLED(TMC_DEBUG)
  #if ENABLED(MONITOR_DRIVER_STATUS)
    void tmc_set_report_status(const bool status);
  #endif
  void tmc_report_all();
#endif

#if TMC_HAS_SPI
  void tmc_init_cs_pins();
#endif
