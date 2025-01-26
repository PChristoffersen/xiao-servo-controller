# BSD 3-Clause License
# 
# Copyright (c) 2025, Peter Christoffersen
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from ctypes import *
import hid
import traceback

from typing import List
from enum import Enum, IntFlag
from dataclasses import dataclass

class LampArrayKind(Enum):
    UNDEFINED = 0x00
    KEYBOARD = 0x01
    MOUSE = 0x02
    GAME_CONTROLLER = 0x03
    PERIPHERAL = 0x04
    SCENE = 0x05
    NOTIFICATION = 0x06
    CHASSIS = 0x07
    WEARABLE = 0x08
    FURNITURE = 0x09
    ART = 0x0A
    VENDOR_DEFINED_FIRST = 0x10000
    VENDOR_DEFINED_LAST = 0xFFFFFFFF

class LampPurpose(Enum):
    CONTROL = 0x00
    ACCENT = 0x01
    BRANDING = 0x02
    STATUS = 0x03
    ILLUMINATION = 0x04
    PRESENTATION = 0x05
    VENDOR_DEFINED_FIRST = 0x10000
    VENDOR_DEFINED_LAST = 0xFFFFFFFF

class LampArrayFlags(IntFlag):
    UPDATE_COMPLETE = (1 << 0)

class LampArrayControlFlags(IntFlag):
    AUTONOMOUS_MODE = (1 << 0)



class LampArrayAttributes(Structure):
    _pack_ = 1
    _fields_ = [
        ("lamp_count", c_uint16),          # Number of lamps associated with this array
        ("width", c_uint32),               # Bounding box width in micrometers
        ("height", c_uint32),              # Bounding box height in micrometers
        ("depth", c_uint32),               # Bounding box depth in micrometers
        ("_kind", c_uint32),                # Kind of lamp array
        ("min_update_interval", c_uint32)  # Minimal update interval in microseconds
    ]

    @property
    def kind(self):
        return LampArrayKind(self._kind)

    @kind.setter
    def kind(self, value):
        self._kind = value.value

    def __repr__(self):
        return (f"LampArrayAttributes(lamp_count={self.lamp_count}, width={self.width}, height={self.height}, "
                f"depth={self.depth}, kind={self.kind}, min_update_interval={self.min_update_interval})")

class LampColor(Structure):
    _pack_ = 1
    _fields_ = [
        ("red", c_uint8),
        ("green", c_uint8),
        ("blue", c_uint8),
        ("intensity", c_uint8)
    ]

    def __repr__(self):
        return (f"LampColor(red={self.red}, green={self.green}, blue={self.blue}, intensity={self.intensity})")


class LampAttributesRequest(Structure):
    _pack_ = 1
    _fields_ = [
        ("lamp_id", c_uint16)
    ]

    def __repr__(self):
        return f"LampAttributesRequest(lamp_id={self.lamp_id})"

class LampAttributes(Structure):
    _pack_ = 1
    _fields_ = [
        ("lamp_id", c_uint16),            # tud_desc_lamp_id_t is uint16_t
        ("position_x", c_uint32),         # x position in micrometers relative to bounding box
        ("position_y", c_uint32),         # y position in micrometers relative to bounding box
        ("position_z", c_uint32),         # z position in micrometers relative to bounding box
        ("update_latency", c_uint32),     # update latency in microseconds
        ("_purpose", c_uint32),           # Purpose of the lamp
        ("red_level_count", c_uint8),     # Number of red levels supported by the lamp
        ("green_level_count", c_uint8),   # Number of green levels supported by the lamp
        ("blue_level_count", c_uint8),    # Number of blue levels supported by the lamp
        ("intensity_level_count", c_uint8),# Number of intensity levels supported by the lamp
        ("is_programmable", c_uint8),     # 1 if the lamp has programmable colors, 0 otherwise
        ("input_binding", c_uint8)        # Keyboard* Usages from the Keyboard/Keypad UsagePage (0x07), or Button* Usages from Button UsagePage (0x09)
    ]

    @property
    def purpose(self):
        return LampPurpose(self._purpose)

    @purpose.setter
    def purpose(self, value):
        self._purpose = value.value

    def __repr__(self):
        return (f"LampAttributesResponse(lamp_id={self.lamp_id}, position_x={self.position_x}, position_y={self.position_y}, "
                f"position_z={self.position_z}, update_latency={self.update_latency}, purpose={self.purpose}, "
                f"red_level_count={self.red_level_count}, green_level_count={self.green_level_count}, "
                f"blue_level_count={self.blue_level_count}, intensity_level_count={self.intensity_level_count}, "
                f"is_programmable={self.is_programmable}, input_binding={self.input_binding})")

class LampMultiUpdate(Structure):
    _pack_ = 1
    _fields_ = [
        ("lamp_count", c_uint8),
        ("flags", c_uint8),
        ("lamp_ids", c_uint16 * 8),       # tud_desc_lamp_id_t is uint16_t
        ("color", LampColor * 8)
    ]

    def __repr__(self):
        return (f"LampMultiUpdate(lamp_count={self.lamp_count}, flags={self.flags}, "
                f"lamp_ids={[self.lamp_ids[i] for i in range(8)]}, "
                f"color={[self.color[i] for i in range(8)]})")

class LampRangeUpdate(Structure):
    _pack_ = 1
    _fields_ = [
        ("flags", c_uint8),
        ("lamp_id_start", c_uint16),      # tud_desc_lamp_id_t is uint16_t
        ("lamp_id_end", c_uint16),        # tud_desc_lamp_id_t is uint16_t
        ("color", LampColor)              # tud_desc_lamp_color_t
    ]

    def __repr__(self):
        return (f"LampRangeUpdate(flags={self.flags}, lamp_id_start={self.lamp_id_start}, "
                f"lamp_id_end={self.lamp_id_end}, color={self.color})")

class LampControl(Structure):
    _pack_ = 1
    _fields_ = [
        ("flags", c_uint8)
    ]

    def __repr__(self):
        return f"LampControl(flags={self.flags})"


class LampArray:
    """
    Very limited implementation of the USB HID protocol for lamp arrays
    """
    DEVICE_VID = 0x2e8a
    DEVICE_PID = 0x4006

    # Report IDs
    # A bit of a hack, the correct way is to parse the report descriptor,
    # but we know the report IDs for this device
    REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES = 2
    REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST = 3
    REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE = 4
    REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE = 5
    REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE = 6
    REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL = 7

    @dataclass
    class _LampData:
        color: LampColor
        dirty: bool


    def __init__(self, vendor_id=DEVICE_VID, product_id=DEVICE_PID, control_on_open=True, release_on_close=True):
        self._vendor_id = vendor_id
        self._product_id = product_id
        self._device = hid.device()
        self._control_on_open = control_on_open
        self._release_on_close = release_on_close
        self._attributes: LampArrayAttributes = None
        self._lamp_attributes: List[LampAttributes] = []
        self._lamp_data: List[LampArray._LampData] = []
        self._show_pending = False

    @property
    def device(self):
        """
        HID device object
        """
        return self._device

    @property
    def attributes(self) -> LampArrayAttributes:
        """
        Lamp array attributes
        """
        return self._attributes
    
    @property
    def lamp_attributes(self) -> List[LampAttributes]:
        """
        List of lamp attributes
        """
        return self._lamp_attributes

    @property
    def n_lamps(self) -> int:
        """
        Number of lamps associated with this array
        """
        return self._attributes.lamp_count

    @property
    def min_update_interval(self) -> float:
        """
        Minimal update interval in seconds
        """
        return self._attributes.min_update_interval / 1e6

    def clear(self, show=False):
        """
        Clear all lamps
        """
        self.fill(LampColor(), show=show)

    def fill(self, color: LampColor, show=False):
        """
        Fill all lamps with the same color
        """
        self.fill_range(0, self._attributes.lamp_count, color, show=show)

    def fill_range(self, id_start: int, count: int, color: LampColor, show=False):
        """
        Fill a range of lamps with the same color
        """
        if show:
            if self._show_pending:
                self.fill_range(int(id_start), int(count), color, show=False)
                self.show()
            else:
                self._range_update(int(id_start), int(id_start+count-1), color, complete=True)                
        else:
            for i in range(int(id_start), int(id_start+count)):
                self._lamp_data[i].color = color
                self._lamp_data[i].dirty = True
            self._show_pending = True

    def set(self, id: int, color: LampColor, show=False):
        """
        Set the color of a lamp
        """
        self._lamp_data[id].color = color
        self._lamp_data[id].dirty = True
        if show:
            self.show()
        else:
            self._show_pending = True

    def get(self, id: int) -> LampColor:
        """
        Get the color of a lamp
        """
        return self._lamp_data[id].color


    def show(self):
        """
        Update all lamps that have been marked as dirty
        """
        self._update()



    def set_autonomous_mode(self, enable: bool):
        """
        Enable or disable autonomous mode
        """
        control = LampControl(flags=LampArrayControlFlags.AUTONOMOUS_MODE if enable else 0)
        self._device.send_feature_report(bytes([LampArray.REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL])+bytes(control))


    def open(self):
        """
        Open the HID device
        """
        self._device.open(self._vendor_id, self._product_id)

        report = self._device.get_feature_report(LampArray.REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES, 64)
        self._attributes = LampArrayAttributes.from_buffer_copy(bytes(report[1:]))

        request = LampAttributesRequest(lamp_id=0)
        self._device.send_feature_report(bytes([LampArray.REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST])+bytes(request))
        for i in range(self._attributes.lamp_count):
            report = self._device.get_feature_report(LampArray.REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE, 64)
            response = LampAttributes.from_buffer_copy(bytes(report[1:]))
            assert response.lamp_id == i
            self._lamp_attributes.append(response)
            self._lamp_data.append(LampArray._LampData(color=LampColor(), dirty=False))
        self._show_pending = False
        if self._control_on_open:
            self.set_autonomous_mode(False)

    def close(self):
        """
        Close the HID device
        """
        if self._release_on_close:
            self.set_autonomous_mode(True)
        self._device.close()
        self._attributes = None
        self._lamp_attributes = []
        self._lamp_data = []


    def _update(self):
        ids = []
        colors = []
        for i in range(self._attributes.lamp_count):
            if self._lamp_data[i].dirty:
                ids.append(i)
                colors.append(self._lamp_data[i].color)
                self._lamp_data[i].dirty = False
        
        if not ids:
            if self._show_pending:
                # There is a pending show, but no updates so send an empty update
                self._multi_update([], [], complete=True)
                self._show_pending = False
            return

        count = len(ids)
        for i in range(0, count, 8):
            chunk = min(8, count-i)
            self._multi_update(ids[i:i+chunk], colors[i:i+chunk], complete=(i+chunk >= count))
        self._show_pending = False


    def _range_update(self, id_start: int, id_end, color: LampColor, complete: bool = True):
        """
        Update a range of lamps with the same color.
        If complete is True the lamps are updated immediately, otherwise the update is deferred.
        """
        assert id_start <= id_end
        assert id_end < self._attributes.lamp_count
        assert id_start >= 0
        for i in range(id_start, id_end+1):
            self._lamp_data[i].color = color
            self._lamp_data[i].dirty = False
        update = LampRangeUpdate(flags=LampArrayFlags.UPDATE_COMPLETE if complete else 0,
                                 lamp_id_start=id_start, lamp_id_end=id_end, color=color)
        self._device.send_feature_report(bytes([LampArray.REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE])+bytes(update))
        self._show_pending = not complete

    def _multi_update(self, ids: List[int], colors: List[LampColor], complete: bool = True):
        """
        Update a list of lamps with different colors
        If complete is True the lamps are updated immediately, otherwise the update is deferred.
        """
        assert len(ids) == len(colors)
        assert len(ids) <= 8
        update = LampMultiUpdate(lamp_count=len(ids), flags=LampArrayFlags.UPDATE_COMPLETE if complete else 0)
        for i in range(len(ids)):
            assert ids[i] < self._attributes.lamp_count
            assert ids[i] >= 0
            update.lamp_ids[i] = ids[i]
            update.color[i] = colors[i]
            self._lamp_data[ids[i]].color = colors[i]
            self._lamp_data[ids[i]].dirty = False
        self._device.send_feature_report(bytes([LampArray.REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE])+bytes(update))
        self._show_pending = not complete



    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        #if exc_type is not None:
        #    traceback.print_exception(exc_type, exc_val, exc_tb)

    def __setitem__(self, key: int, color: LampColor):
        """
        Set the color of a lamp
        """
        self.set(key, color)

    def __getitem__(self, key: int) -> LampColor:
        """
        Get the color of a lamp
        """
        return self.get(key)

    def __len__(self):
        """
        Number of lamps associated with this array
        """
        return self._attributes.lamp_count
