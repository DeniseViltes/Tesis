import io
import struct
from dataclasses import dataclass
from enum import Enum

import usb

from compensator import Compensator

chVv = {
        0: 0.002,
        1: 0.005,
        2: 0.01,
        3: 0.02,
        4: 0.05,
        5: 0.1,
        6: 0.2,
        7: 0.5,
        8: 1.0,
        9: 2.0,
        10: 5.0,
        11: 10.0,
        12: 20.0,
        13: 50.0,
        14: 100.0,
        15: 200.0,
        16: 500.0,
        17: 1000.0,
        18: 2000.0,
        19: 5000.0,
        20: 10000.0
    }

chHs_V = {
    -2: 0.000001,
    -1: 0.000002,
    0: 0.000005,
    1: 0.00001,
    2: 0.000025,
    3: 0.00005,
    4: 0.0001,
    5: 0.00025,
    6: 0.0005,
    7: 0.001,
    8: 0.0025,
    9: 0.005,
    10: 0.01,
    11: 0.025,
    12: 0.05,
    13: 0.1,
    14: 0.25,
    15: 0.5,
    16: 1,
    17: 2.5,
    18: 5,
    19: 10,
    20: 25,
    21: 50,
    22: 100,
    23: 250,
    24: 500,
    25: 1000,
    26: 2500,
    27: 5000,
    28: 10000,
    29: 25000,
    30: 50000,
    31: 100000,
}

chHs = {
    -2: 0.000001,
    -1: 0.000002,
    0: 0.000005,
    1: 0.00001,
    2: 0.00002,
    3: 0.00005,
    4: 0.0001,
    5: 0.0002,
    6: 0.0005,
    7: 0.001,
    8: 0.002,
    9: 0.005,
    10: 0.01,
    11: 0.02,
    12: 0.05,
    13: 0.1,
    14: 0.2,
    15: 0.5,
    16: 1,
    17: 2.,
    18: 5,
    19: 10,
    20: 20,
    21: 50,
    22: 100,
    23: 200,
    24: 500,
    25: 1000,
    26: 2000,
    27: 5000,
    28: 10000,
    29: 20000,
    30: 50000,
    31: 100000,
}
@dataclass
class ResponseStartLength:
    length_of_data: int
    placeholder: int
    flag: int

    def __init__(self, data: bytearray):
        self.length_of_data, self.placeholder, self.flag = struct.unpack('<iii', data)
        #print(self.length_of_data, self.flag)

@dataclass
class ResponseBin:
    class WaveTypeEnum(Enum):
        Normal = 0
        DeepMemory = 1

    machine_model: str
    waveform_name: str
    block_length: int
    whole_screen_collecting_points: int
    number_of_collecting_points: int
    slow_moving_number: int
    time_base_level: int
    zero_point: int
    voltage_level: int
    attenuation_multiplying_power_index: int
    spacing_interval: float
    frequency: int
    cycle: int
    voltage_value_per_point: float
    sampling_data: list


    #optionals
    extended_value: int = 0
    wave_type: WaveTypeEnum = WaveTypeEnum.Normal
    has_deep_memory: bool = False

    def __init__(self, data, machine_model):
        stream=io.BytesIO(data)
        self.machine_model = machine_model
        self.waveform_name = stream.read(3).decode('ascii')
        self.block_length = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        if self.block_length < 0:
            self.extended_value = int.from_bytes(stream.read(4), byteorder='little', signed=True)
            self.wave_type = self.WaveTypeEnum(self.extended_value & 0x1)
            self.has_deep_memory = bool(self.extended_value & 0x2)

            if machine_model.startswith("SPBS"):
                stream.read(4)
        self.whole_screen_collecting_points = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.number_of_collecting_points = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.slow_moving_number = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.time_base_level = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.zero_point = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.voltage_level = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.attenuation_multiplying_power_index = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.spacing_interval = struct.unpack('<f', stream.read(4))[0]
        self.frequency = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.cycle = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        self.voltage_value_per_point = struct.unpack('<f', stream.read(4))[0]

        if self.wave_type == self.WaveTypeEnum.Normal:
            # Normal wave, 2 bytes array, short[]
            self.sampling_data = list(struct.unpack('<' + 'h' * (self.number_of_collecting_points),
                                                    stream.read(2 * self.number_of_collecting_points)))
        elif self.wave_type == self.WaveTypeEnum.DeepMemory:
            # Deep memory wave, 1 byte array, byte[]
            self.sampling_data = list(struct.unpack('<' + 'b' * (self.number_of_collecting_points),
                                                    stream.read(self.number_of_collecting_points)))
        with open("/tmp/remaining.bin", "wb") as fo:
            fo.write(stream.read())

        #parser = struct.Struct('<6si3si')
        #self.machine_model, self.file_length, self.waveform_name, self.block_length = parser.unpack(data[:parser.size])
        #print(parser.size)
        #print(self)

    @property
    def volts_per_div(self):
        return chVv[self.voltage_level]

    @property
    def milliseconds_per_div(self):
        if self.machine_model.startswith("SPBM") or self.machine_model in ("SPBS03", "SPBS04"):
            offset = -2
        elif self.machine_model.startswith("SPBN") or self.machine_model in ("SPCX01", ):
            offset = -1
        else:
            offset = 0

        time_map = chHs_V if self.machine_model[-3] == "V" else chHs
        return time_map[self.time_base_level + offset]

    @property
    def calculated_spacing_interval(self):
        # magia no documentada, podria andar solo para "SDS6062"
        if (self.time_base_level  > 5):
            scale = 25 if (self.time_base_level % 3 ) == 2 else 20
        else:
            scale = 10 if (self.time_base_level % 3 ) == 2 else 5
        return self.milliseconds_per_div * 100 / scale

@dataclass(kw_only=True)
class ResponseBinWithHeader(ResponseBin):

    file_length: int #probably wrong
    def __init__(self, data):
        stream = io.BytesIO(data)
        machine_model = stream.read(6).decode('ascii')
        self.file_length = int.from_bytes(stream.read(4), byteorder='little', signed=True)
        super().__init__(data[10:], machine_model)


class OwonOsciloscope:
    def __init__(self):
        dev = usb.core.find(idVendor=0x5345, idProduct=0x1234, product="Oscillope")
        if dev is None:
            raise Exception("Device not found")
        self.dev = dev

        # set the active configuration. With no arguments, the first
        # configuration will be the active one
        self.dev.set_configuration()

        # get an endpoint instance
        cfg = self.dev.get_active_configuration()
        intf = cfg[(0, 0)]

        #Get Bulk communication endpoints
        self.ep_w = usb.util.find_descriptor(
            intf,
            # match the first OUT endpoint
            custom_match= \
                lambda e: \
                    usb.util.endpoint_direction(e.bEndpointAddress) == \
                    usb.util.ENDPOINT_OUT)

        self.ep_r = usb.util.find_descriptor(
            intf,
            # match the first OUT endpoint
            custom_match= \
                lambda e: \
                    usb.util.endpoint_direction(e.bEndpointAddress) == \
                    usb.util.ENDPOINT_IN)

        self.compensator = {
            "CH1": Compensator("CH1_calibration.csv"),
            "CH2": Compensator("CH2_calibration.csv")
        }
    def _write(self, data):
        self.ep_w.write(data)

    def _read(self):
        #Read Header
        rsl = ResponseStartLength(self.ep_r.read(12))
        return rsl, self.ep_r.read(rsl.length_of_data)

    def ReadBmp(self):
        self._write('STARTBMP')
        rsl, data = self._read()
        return data

    def ReadBin(self):
        self._write('STARTBIN')
        rsl, data = self._read()
        response_with_header = ResponseBinWithHeader(data)
        responses = [response_with_header,]
        while rsl.flag > 128:
            rsl, data = self._read()
            responses.append(ResponseBin(data, response_with_header.machine_model))
        return responses

    def ReadSignal(self):
        responses = self.ReadBin()
        return [(self._extract_time_base(rsp),  self._extract_signal(rsp)) for rsp in responses]

    def ReadSignalWithCompensation(self):
        responses = self.ReadBin()
        return [(self._extract_time_base(rsp), self.compensator[rsp.waveform_name].compensate(self._extract_signal(rsp))) for rsp in responses]
    def _extract_signal(self, rsp: ResponseBin):
        #Convert to Volts
        alpha = rsp.voltage_value_per_point * 10 ** rsp.attenuation_multiplying_power_index / 1000
        return [point * alpha for point in rsp.sampling_data]

    def _extract_time_base(self, rsp: ResponseBin):
        step = rsp.calculated_spacing_interval
        return [n * step for n in range(len(rsp.sampling_data))]

    def close(self):
        self.dev.reset()
        usb.util.dispose_resources(self.dev)

    def calc_mean_without_compensation(self):
        signals = self.ReadSignal()
        return [ sum(signal[1])/len(signal[1]) for signal in signals]

    def calc_mean(self):
        signals = self.ReadSignalWithCompensation()
        return [ sum(signal[1])/len(signal[1]) for signal in signals]
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
