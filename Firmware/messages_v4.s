	; BPMSG1001
	.section .text.BPMSG1001, code
	.global _BPMSG1001_str
_BPMSG1001_str:
	.pascii " *next clock (^) will use this value"

	; BPMSG1004
	.section .text.BPMSG1004, code
	.global _BPMSG1004_str
_BPMSG1004_str:
	.pascii "No device, try (ALARM) SEARCH macro first"

	; BPMSG1005
	.section .text.BPMSG1005, code
	.global _BPMSG1005_str
_BPMSG1005_str:
	.pascii "ADDRESS MACRO "

	; BPMSG1006
	.section .text.BPMSG1006, code
	.global _BPMSG1006_str
_BPMSG1006_str:
	.pascii " 0.Macro menu"

	; BPMSG1007
	.section .text.BPMSG1007, code
	.global _BPMSG1007_str
_BPMSG1007_str:
	.pascii "Macro     1WIRE address"

	; BPMSG1008
	.section .text.BPMSG1008, code
	.global _BPMSG1008_str
_BPMSG1008_str:
	.pascii "\r\n   *"

	; BPMSG1009
	.section .text.BPMSG1009, code
	.global _BPMSG1009_str
_BPMSG1009_str:
	.pascii "1WIRE ROM COMMAND MACROs:\r\n 51.READ ROM (0x33) *for single device bus\r\n 85.MATCH ROM (0x55) *followed by 64bit address\r\n 204.SKIP ROM (0xCC) *followed by command\r\n 236.ALARM SEARCH (0xEC)\r\n 240.SEARCH ROM (0xF0)"

	; BPMSG1010
	.section .text.BPMSG1010, code
	.global _BPMSG1010_str
_BPMSG1010_str:
	.pascii "ALARM SEARCH (0xEC)"

	; BPMSG1011
	.section .text.BPMSG1011, code
	.global _BPMSG1011_str
_BPMSG1011_str:
	.pascii "SEARCH (0xF0)"

	; BPMSG1012
	.section .text.BPMSG1012, code
	.global _BPMSG1012_str
_BPMSG1012_str:
	.pascii "Device IDs are available by MACRO, see (0)."

	; BPMSG1013
	.section .text.BPMSG1013, code
	.global _BPMSG1013_str
_BPMSG1013_str:
	.pascii "READ ROM (0x33): "

	; BPMSG1014
	.section .text.BPMSG1014, code
	.global _BPMSG1014_str
_BPMSG1014_str:
	.pascii "MATCH ROM (0x55)"

	; BPMSG1015
	.section .text.BPMSG1015, code
	.global _BPMSG1015_str
_BPMSG1015_str:
	.pascii "SKIP ROM (0xCC)"

	; BPMSG1017
	.section .text.BPMSG1017, code
	.global _BPMSG1017_str
_BPMSG1017_str:
	.pascii "BUS RESET "

	; BPMSG1019
	.section .text.BPMSG1019, code
	.global _BPMSG1019_str
_BPMSG1019_str:
	.pascii "Warning: "

	; BPMSG1020
	.section .text.BPMSG1020, code
	.global _BPMSG1020_str
_BPMSG1020_str:
	.pascii "*Short or no pull-up "

	; BPMSG1021
	.section .text.BPMSG1021, code
	.global _BPMSG1021_str
_BPMSG1021_str:
	.pascii "*No device detected "

	; BPMSG1022
	.section .text.BPMSG1022, code
	.global _BPMSG1022_str
_BPMSG1022_str:
	.pascii "DS18S20 High Pres Dig Therm"

	; BPMSG1023
	.section .text.BPMSG1023, code
	.global _BPMSG1023_str
_BPMSG1023_str:
	.pascii "DS18B20 Prog Res Dig Therm"

	; BPMSG1024
	.section .text.BPMSG1024, code
	.global _BPMSG1024_str
_BPMSG1024_str:
	.pascii "DS1822 Econ Dig Therm"

	; BPMSG1025
	.section .text.BPMSG1025, code
	.global _BPMSG1025_str
_BPMSG1025_str:
	.pascii "DS2404 Econram time Chip"

	; BPMSG1026
	.section .text.BPMSG1026, code
	.global _BPMSG1026_str
_BPMSG1026_str:
	.pascii "DS2431 1K EEPROM"

	; BPMSG1027
	.section .text.BPMSG1027, code
	.global _BPMSG1027_str
_BPMSG1027_str:
	.pascii "Unknown device"

	; BPMSG1028
	.section .text.BPMSG1028, code
	.global _BPMSG1028_str
_BPMSG1028_str:
	.pascii "PWM disabled"

	; BPMSG1029
	.section .text.BPMSG1029, code
	.global _BPMSG1029_str
_BPMSG1029_str:
	.pascii "1KHz-4,000KHz PWM"

	; BPMSG1030
	.section .text.BPMSG1030, code
	.global _BPMSG1030_str
_BPMSG1030_str:
	.pascii "Frequency in KHz "

	; BPMSG1033
	.section .text.BPMSG1033, code
	.global _BPMSG1033_str
_BPMSG1033_str:
	.pascii "Duty cycle in % "

	; BPMSG1034
	.section .text.BPMSG1034, code
	.global _BPMSG1034_str
_BPMSG1034_str:
	.pascii "PWM active"

	; BPMSG1037
	.section .text.BPMSG1037, code
	.global _BPMSG1037_str
_BPMSG1037_str:
	.pascii "ERROR: PWM active, g to disable"

	; BPMSG1038
	.section .text.BPMSG1038, code
	.global _BPMSG1038_str
_BPMSG1038_str:
	.pascii "AUX Frequency: "

	; BPMSG1039
	.section .text.BPMSG1039, code
	.global _BPMSG1039_str
_BPMSG1039_str:
	.pascii "AUX INPUT/HI-Z"

	; BPMSG1040
	.section .text.BPMSG1040, code
	.global _BPMSG1040_str
_BPMSG1040_str:
	.pascii "AUX HIGH"

	; BPMSG1041
	.section .text.BPMSG1041, code
	.global _BPMSG1041_str
_BPMSG1041_str:
	.pascii "AUX LOW"

	; BPMSG1042
	.section .text.BPMSG1042, code
	.global _BPMSG1042_str
_BPMSG1042_str:
	.pascii "VOLTMETER MODE"

	; BPMSG1044
	.section .text.BPMSG1044, code
	.global _BPMSG1044_str
_BPMSG1044_str:
	.pascii "VOLTAGE PROBE: "

	; BPMSG1045
	.section .text.BPMSG1045, code
	.global _BPMSG1045_str
_BPMSG1045_str:
	.pascii "V"

	; BPMSG1047
	.section .text.BPMSG1047, code
	.global _BPMSG1047_str
_BPMSG1047_str:
	.pascii "Error("

	; BPMSG1048
	.section .text.BPMSG1048, code
	.global _BPMSG1048_str
_BPMSG1048_str:
	.pascii ") @line:"

	; BPMSG1049
	.section .text.BPMSG1049, code
	.global _BPMSG1049_str
_BPMSG1049_str:
	.pascii " @pgmspace:"

	; BPMSG1050
	.section .text.BPMSG1050, code
	.global _BPMSG1050_str
_BPMSG1050_str:
	.pascii " bytes."

	; BPMSG1051
	.section .text.BPMSG1051, code
	.global _BPMSG1051_str
_BPMSG1051_str:
	.pascii "Too long!"

	; BPMSG1052
	.section .text.BPMSG1052, code
	.global _BPMSG1052_str
_BPMSG1052_str:
	.pascii "Syntax error"

	; BPMSG1053
	.section .text.BPMSG1053, code
	.global _BPMSG1053_str
_BPMSG1053_str:
	.pascii "No EEPROM"

	; BPMSG1054
	.section .text.BPMSG1054, code
	.global _BPMSG1054_str
_BPMSG1054_str:
	.pascii "Erasing"

	; BPMSG1055
	.section .text.BPMSG1055, code
	.global _BPMSG1055_str
_BPMSG1055_str:
	.pascii "done"

	; BPMSG1056
	.section .text.BPMSG1056, code
	.global _BPMSG1056_str
_BPMSG1056_str:
	.pascii "Saving to slot "

	; BPMSG1057
	.section .text.BPMSG1057, code
	.global _BPMSG1057_str
_BPMSG1057_str:
	.pascii "Invalid slot"

	; BPMSG1058
	.section .text.BPMSG1058, code
	.global _BPMSG1058_str
_BPMSG1058_str:
	.pascii "Loading from slot "

	; BPMSG1059
	.section .text.BPMSG1059, code
	.global _BPMSG1059_str
_BPMSG1059_str:
	.pascii "ERROR: command has no effect here"

	; BPMSG1064
	.section .text.BPMSG1064, code
	.global _BPMSG1064_str
_BPMSG1064_str:
	.pascii "I2C mode:\r\n 1. Software\r\n 2. Hardware"

	; BPMSG1067
	.section .text.BPMSG1067, code
	.global _BPMSG1067_str
_BPMSG1067_str:
	.pascii "Set speed:\r\n 1. 100KHz\r\n 2. 400KHz\r\n 3. 1MHz"

	; BPMSG1068
	.section .text.BPMSG1068, code
	.global _BPMSG1068_str
_BPMSG1068_str:
	.pascii "I2C (mod spd)=( "

	; BPMSG1069
	.section .text.BPMSG1069, code
	.global _BPMSG1069_str
_BPMSG1069_str:
	.pascii " 0.Macro menu\r\n 1.7bit address search\r\n 2.I2C sniffer\r\n 3.Connect to on-board EEPROM\r\n 4.Enable Writing the on-board EEPROM"

	; BPMSG1070
	.section .text.BPMSG1070, code
	.global _BPMSG1070_str
_BPMSG1070_str:
	.pascii "Searching I2C address space. Found devices at:"

	; BPMSG1071
	.section .text.BPMSG1071, code
	.global _BPMSG1071_str
_BPMSG1071_str:
	.pascii "Sniffer"

	; BPMSG1072
	.section .text.BPMSG1072, code
	.global _BPMSG1072_str
_BPMSG1072_str:
	.pascii "Commandmode?\r\n1. 6b/14b\r\n2. 4b/16b"

	; BPMSG1073
	.section .text.BPMSG1073, code
	.global _BPMSG1073_str
_BPMSG1073_str:
	.pascii "Delay?"

	; BPMSG1074
	.section .text.BPMSG1074, code
	.global _BPMSG1074_str
_BPMSG1074_str:
	.pascii "PIC(mod dly)=("

	; BPMSG1075
	.section .text.BPMSG1075, code
	.global _BPMSG1075_str
_BPMSG1075_str:
	.pascii "CMD"

	; BPMSG1076
	.section .text.BPMSG1076, code
	.global _BPMSG1076_str
_BPMSG1076_str:
	.pascii "DTA"

	; BPMSG1077
	.section .text.BPMSG1077, code
	.global _BPMSG1077_str
_BPMSG1077_str:
	.pascii "no read"

	; BPMSG1078
	.section .text.BPMSG1078, code
	.global _BPMSG1078_str
_BPMSG1078_str:
	.pascii "unknown mode"

	; BPMSG1079
	.section .text.BPMSG1079, code
	.global _BPMSG1079_str
_BPMSG1079_str:
	.pascii "(1) get devID"

	; BPMSG1080
	.section .text.BPMSG1080, code
	.global _BPMSG1080_str
_BPMSG1080_str:
	.pascii "DevID = "

	; BPMSG1081
	.section .text.BPMSG1081, code
	.global _BPMSG1081_str
_BPMSG1081_str:
	.pascii " Rev = "

	; BPMSG1082
	.section .text.BPMSG1082, code
	.global _BPMSG1082_str
_BPMSG1082_str:
	.pascii "Not implemented (yet)"

	; BPMSG1083
	.section .text.BPMSG1083, code
	.global _BPMSG1083_str
_BPMSG1083_str:
	.pascii "Please exit PIC programming mode"

	; BPMSG1084
	.section .text.BPMSG1084, code
	.global _BPMSG1084_str
_BPMSG1084_str:
	.pascii "(BASIC)"

	; BPMSG1085
	.section .text.BPMSG1085, code
	.global _BPMSG1085_str
_BPMSG1085_str:
	.pascii "Ready"

	; BPMSG1086
	.section .text.BPMSG1086, code
	.global _BPMSG1086_str
_BPMSG1086_str:
	.pascii "a/A/@ controls AUX pin"

	; BPMSG1087
	.section .text.BPMSG1087, code
	.global _BPMSG1087_str
_BPMSG1087_str:
	.pascii "a/A/@ controls CS pin"

	; BPMSG1088
	.section .text.BPMSG1088, code
	.global _BPMSG1088_str
_BPMSG1088_str:
	.pascii "Command not used in this mode"

	; BPMSG1089
	.section .text.BPMSG1089, code
	.global _BPMSG1089_str
_BPMSG1089_str:
	.pascii "Pull-up resistors OFF"

	; BPMSG1091
	.section .text.BPMSG1091, code
	.global _BPMSG1091_str
_BPMSG1091_str:
	.pascii "Pull-up resistors ON"

	; BPMSG1092
	.section .text.BPMSG1092, code
	.global _BPMSG1092_str
_BPMSG1092_str:
	.pascii "Self-test in HiZ mode only"

	; BPMSG1093
	.section .text.BPMSG1093, code
	.global _BPMSG1093_str
_BPMSG1093_str:
	.pascii "RESET"

	; BPMSG1094
	.section .text.BPMSG1094, code
	.global _BPMSG1094_str
_BPMSG1094_str:
	.pascii "BOOTLOADER"

	; BPMSG1095
	.section .text.BPMSG1095, code
	.global _BPMSG1095_str
_BPMSG1095_str:
	.pascii "AUX INPUT/HI-Z, READ: "

	; BPMSG1096
	.section .text.BPMSG1096, code
	.global _BPMSG1096_str
_BPMSG1096_str:
	.pascii "POWER SUPPLIES ON"

	; BPMSG1097
	.section .text.BPMSG1097, code
	.global _BPMSG1097_str
_BPMSG1097_str:
	.pascii "POWER SUPPLIES OFF"

	; BPMSG1098
	.section .text.BPMSG1098, code
	.global _BPMSG1098_str
_BPMSG1098_str:
	.pascii "DATA STATE: "

	; BPMSG1099
	.section .text.BPMSG1099, code
	.global _BPMSG1099_str
_BPMSG1099_str:
	.pascii "DELAY "

	; BPMSG1100
	.section .text.BPMSG1100, code
	.global _BPMSG1100_str
_BPMSG1100_str:
	.pascii "us"

	; BPMSG1101
	.section .text.BPMSG1101, code
	.global _BPMSG1101_str
_BPMSG1101_str:
	.pascii "WRITE: "

	; BPMSG1102
	.section .text.BPMSG1102, code
	.global _BPMSG1102_str
_BPMSG1102_str:
	.pascii "READ: "

	; BPMSG1103
	.section .text.BPMSG1103, code
	.global _BPMSG1103_str
_BPMSG1103_str:
	.pascii "CLOCK, 1"

	; BPMSG1104
	.section .text.BPMSG1104, code
	.global _BPMSG1104_str
_BPMSG1104_str:
	.pascii "CLOCK, 0"

	; BPMSG1105
	.section .text.BPMSG1105, code
	.global _BPMSG1105_str
_BPMSG1105_str:
	.pascii "DATA OUTPUT, 1"

	; BPMSG1106
	.section .text.BPMSG1106, code
	.global _BPMSG1106_str
_BPMSG1106_str:
	.pascii "DATA OUTPUT, 0"

	; BPMSG1107
	.section .text.BPMSG1107, code
	.global _BPMSG1107_str
_BPMSG1107_str:
	.pascii " *pin is now HiZ"

	; BPMSG1108
	.section .text.BPMSG1108, code
	.global _BPMSG1108_str
_BPMSG1108_str:
	.pascii "CLOCK TICKS: "

	; BPMSG1109
	.section .text.BPMSG1109, code
	.global _BPMSG1109_str
_BPMSG1109_str:
	.pascii "READ BIT: "

	; BPMSG1110
	.section .text.BPMSG1110, code
	.global _BPMSG1110_str
_BPMSG1110_str:
	.pascii "Syntax error at char "

	; BPMSG1111
	.section .text.BPMSG1111, code
	.global _BPMSG1111_str
_BPMSG1111_str:
	.pascii "x. exit(without change)"

	; BPMSG1112
	.section .text.BPMSG1112, code
	.global _BPMSG1112_str
_BPMSG1112_str:
	.pascii "no mode change"

	; BPMSG1114
	.section .text.BPMSG1114, code
	.global _BPMSG1114_str
_BPMSG1114_str:
	.pascii "Nonexistent protocol!"

	; BPMSG1115
	.section .text.BPMSG1115, code
	.global _BPMSG1115_str
_BPMSG1115_str:
	.pascii "x. exit"

	; BPMSG1117
	.section .text.BPMSG1117, code
	.global _BPMSG1117_str
_BPMSG1117_str:
	.pascii "DEVID:"

	; BPMSG1118
	.section .text.BPMSG1118, code
	.global _BPMSG1118_str
_BPMSG1118_str:
	.pascii "http://dangerousprototypes.com"

	; BPMSG1119
	.section .text.BPMSG1119, code
	.global _BPMSG1119_str
_BPMSG1119_str:
	.pascii "*----------*"

	; BPMSG1120
	.section .text.BPMSG1120, code
	.global _BPMSG1120_str
_BPMSG1120_str:
	.pascii "Open drain outputs (H=Hi-Z, L=GND)"

	; BPMSG1121
	.section .text.BPMSG1121, code
	.global _BPMSG1121_str
_BPMSG1121_str:
	.pascii "Normal outputs (H=3.3v, L=GND)"

	; BPMSG1123
	.section .text.BPMSG1123, code
	.global _BPMSG1123_str
_BPMSG1123_str:
	.pascii "MSB set: MOST sig bit first"

	; BPMSG1124
	.section .text.BPMSG1124, code
	.global _BPMSG1124_str
_BPMSG1124_str:
	.pascii "LSB set: LEAST sig bit first"

	; BPMSG1127
	.section .text.BPMSG1127, code
	.global _BPMSG1127_str
_BPMSG1127_str:
	.pascii " 1. HEX\r\n 2. DEC\r\n 3. BIN\r\n 4. RAW"

	; BPMSG1128
	.section .text.BPMSG1128, code
	.global _BPMSG1128_str
_BPMSG1128_str:
	.pascii "Display format set"

	; BPMSG1133
	.section .text.BPMSG1133, code
	.global _BPMSG1133_str
_BPMSG1133_str:
	.pascii "Set serial port speed: (bps)\r\n 1. 300\r\n 2. 1200\r\n 3. 2400\r\n 4. 4800\r\n 5. 9600\r\n 6. 19200\r\n 7. 38400\r\n 8. 57600\r\n 9. 115200\r\n10. Input Custom BAUD\r\n11. Auto-Baud Detection (Activity Required)"

	; BPMSG1134
	.section .text.BPMSG1134, code
	.global _BPMSG1134_str
_BPMSG1134_str:
	.pascii "Adjust your terminal"

	; BPMSG1135
	.section .text.BPMSG1135, code
	.global _BPMSG1135_str
_BPMSG1135_str:
	.pascii "Are you sure? "

	; BPMSG1136
	.section .text.BPMSG1136, code
	.global _BPMSG1136_str
_BPMSG1136_str:
	.pascii "CFG1:"

	; BPMSG1137
	.section .text.BPMSG1137, code
	.global _BPMSG1137_str
_BPMSG1137_str:
	.pascii " CFG2:"

	; BPMSG1163
	.section .text.BPMSG1163, code
	.global _BPMSG1163_str
_BPMSG1163_str:
	.pascii "Disconnect any devices\r\nConnect (ADC to +3.3V)"

	; BPMSG1164
	.section .text.BPMSG1164, code
	.global _BPMSG1164_str
_BPMSG1164_str:
	.pascii "Ctrl"

	; BPMSG1165
	.section .text.BPMSG1165, code
	.global _BPMSG1165_str
_BPMSG1165_str:
	.pascii "AUX"

	; BPMSG1166
	.section .text.BPMSG1166, code
	.global _BPMSG1166_str
_BPMSG1166_str:
	.pascii "MODE LED"

	; BPMSG1167
	.section .text.BPMSG1167, code
	.global _BPMSG1167_str
_BPMSG1167_str:
	.pascii "PULLUP H"

	; BPMSG1168
	.section .text.BPMSG1168, code
	.global _BPMSG1168_str
_BPMSG1168_str:
	.pascii "PULLUP L"

	; BPMSG1169
	.section .text.BPMSG1169, code
	.global _BPMSG1169_str
_BPMSG1169_str:
	.pascii "VREG"

	; BPMSG1170
	.section .text.BPMSG1170, code
	.global _BPMSG1170_str
_BPMSG1170_str:
	.pascii "ADC and supply"

	; BPMSG1171
	.section .text.BPMSG1171, code
	.global _BPMSG1171_str
_BPMSG1171_str:
	.pascii "5V"

	; BPMSG1172
	.section .text.BPMSG1172, code
	.global _BPMSG1172_str
_BPMSG1172_str:
	.pascii "VPU"

	; BPMSG1173
	.section .text.BPMSG1173, code
	.global _BPMSG1173_str
_BPMSG1173_str:
	.pascii "3.3V"

	; BPMSG1174
	.section .text.BPMSG1174, code
	.global _BPMSG1174_str
_BPMSG1174_str:
	.pascii "ADC"

	; BPMSG1175
	.section .text.BPMSG1175, code
	.global _BPMSG1175_str
_BPMSG1175_str:
	.pascii "Bus high"

	; BPMSG1176
	.section .text.BPMSG1176, code
	.global _BPMSG1176_str
_BPMSG1176_str:
	.pascii "Bus Hi-Z 0"

	; BPMSG1177
	.section .text.BPMSG1177, code
	.global _BPMSG1177_str
_BPMSG1177_str:
	.pascii "Bus Hi-Z 1"

	; BPMSG1178
	.section .text.BPMSG1178, code
	.global _BPMSG1178_str
_BPMSG1178_str:
	.pascii "MODE, VREG, and USB LEDs should be on!"

	; BPMSG1179
	.section .text.BPMSG1179, code
	.global _BPMSG1179_str
_BPMSG1179_str:
	.pascii "Found "

	; BPMSG1180
	.section .text.BPMSG1180, code
	.global _BPMSG1180_str
_BPMSG1180_str:
	.pascii " errors."

	; BPMSG1181
	.section .text.BPMSG1181, code
	.global _BPMSG1181_str
_BPMSG1181_str:
	.pascii "MOSI"

	; BPMSG1182
	.section .text.BPMSG1182, code
	.global _BPMSG1182_str
_BPMSG1182_str:
	.pascii "CLK"

	; BPMSG1183
	.section .text.BPMSG1183, code
	.global _BPMSG1183_str
_BPMSG1183_str:
	.pascii "MISO"

	; BPMSG1184
	.section .text.BPMSG1184, code
	.global _BPMSG1184_str
_BPMSG1184_str:
	.pascii "CS"

	; BPMSG1185
	.section .text.BPMSG1185, code
	.global _BPMSG1185_str
_BPMSG1185_str:
	.pascii " OK"

	; BPMSG1186
	.section .text.BPMSG1186, code
	.global _BPMSG1186_str
_BPMSG1186_str:
	.pascii " FAIL"

	; BPMSG1187
	.section .text.BPMSG1187, code
	.global _BPMSG1187_str
_BPMSG1187_str:
	.pascii "Set speed:\r\n 1.  30KHz\r\n 2. 125KHz\r\n 3. 250KHz\r\n 4.   1MHz\r\n 5.  50KHz\r\n 6. 1.3MHz\r\n 7.   2MHz\r\n 8. 2.6MHz\r\n 9. 3.2MHz\r\n10.   4MHz\r\n11. 5.3MHz\r\n12.   8MHz"

	; BPMSG1188
	.section .text.BPMSG1188, code
	.global _BPMSG1188_str
_BPMSG1188_str:
	.pascii "Clock polarity:\r\n 1. Idle low *default\r\n 2. Idle high"

	; BPMSG1189
	.section .text.BPMSG1189, code
	.global _BPMSG1189_str
_BPMSG1189_str:
	.pascii "Output clock edge:\r\n 1. Idle to active\r\n 2. Active to idle *default"

	; BPMSG1190
	.section .text.BPMSG1190, code
	.global _BPMSG1190_str
_BPMSG1190_str:
	.pascii "Input sample phase:\r\n 1. Middle *default\r\n 2. End"

	; BPMSG1191
	.section .text.BPMSG1191, code
	.global _BPMSG1191_str
_BPMSG1191_str:
	.pascii "SPI (spd ckp ske smp csl hiz)=( "

	; BPMSG1192
	.section .text.BPMSG1192, code
	.global _BPMSG1192_str
_BPMSG1192_str:
	.pascii " 0.Macro menu\r\n 1.Sniff CS low\r\n 2.Sniff all traffic"

	; BPMSG1194
	.section .text.BPMSG1194, code
	.global _BPMSG1194_str
_BPMSG1194_str:
	.pascii "-p "

	; BPMSG1195
	.section .text.BPMSG1195, code
	.global _BPMSG1195_str
_BPMSG1195_str:
	.pascii "-f "

	; BPMSG1196
	.section .text.BPMSG1196, code
	.global _BPMSG1196_str
_BPMSG1196_str:
	.pascii "*Bytes dropped*"

	; BPMSG1197
	.section .text.BPMSG1197, code
	.global _BPMSG1197_str
_BPMSG1197_str:
	.pascii "FAILED, NO DATA"

	; BPMSG1199
	.section .text.BPMSG1199, code
	.global _BPMSG1199_str
_BPMSG1199_str:
	.pascii "Data bits and parity:\r\n 1. 8, NONE *default \r\n 2. 8, EVEN \r\n 3. 8, ODD \r\n 4. 9, NONE"

	; BPMSG1200
	.section .text.BPMSG1200, code
	.global _BPMSG1200_str
_BPMSG1200_str:
	.pascii "Stop bits:\r\n 1. 1 *default\r\n 2. 2"

	; BPMSG1201
	.section .text.BPMSG1201, code
	.global _BPMSG1201_str
_BPMSG1201_str:
	.pascii "Receive polarity:\r\n 1. Idle 1 *default\r\n 2. Idle 0"

	; BPMSG1202
	.section .text.BPMSG1202, code
	.global _BPMSG1202_str
_BPMSG1202_str:
	.pascii "UART (spd brg dbp sb rxp hiz)=( "

	; BPMSG1203
	.section .text.BPMSG1203, code
	.global _BPMSG1203_str
_BPMSG1203_str:
	.pascii " 0.Macro menu\r\n 1.Transparent bridge\r\n 2.Live monitor\r\n 3.Bridge with flow control\n\r 4.Auto Baud Detection (Activity Needed)"

	; BPMSG1204
	.section .text.BPMSG1204, code
	.global _BPMSG1204_str
_BPMSG1204_str:
	.pascii "UART bridge"

	; BPMSG1206
	.section .text.BPMSG1206, code
	.global _BPMSG1206_str
_BPMSG1206_str:
	.pascii "Raw UART input"

	; BPMSG1207
	.section .text.BPMSG1207, code
	.global _BPMSG1207_str
_BPMSG1207_str:
	.pascii "UART LIVE DISPLAY, } TO STOP"

	; BPMSG1208
	.section .text.BPMSG1208, code
	.global _BPMSG1208_str
_BPMSG1208_str:
	.pascii "LIVE DISPLAY STOPPED"

	; BPMSG1209
	.section .text.BPMSG1209, code
	.global _BPMSG1209_str
_BPMSG1209_str:
	.pascii "WARNING: pins not open drain (HiZ)"

	; BPMSG1210
	.section .text.BPMSG1210, code
	.global _BPMSG1210_str
_BPMSG1210_str:
	.pascii " REVID:"

	; BPMSG1211
	.section .text.BPMSG1211, code
	.global _BPMSG1211_str
_BPMSG1211_str:
	.pascii "\r\nInvalid choice, try again"

	; BPMSG1212
	.section .text.BPMSG1212, code
	.global _BPMSG1212_str
_BPMSG1212_str:
	.pascii "ms"

	; BPMSG1213
	.section .text.BPMSG1213, code
	.global _BPMSG1213_str
_BPMSG1213_str:
	.pascii "RS LOW, COMMAND MODE"

	; BPMSG1214
	.section .text.BPMSG1214, code
	.global _BPMSG1214_str
_BPMSG1214_str:
	.pascii "RS HIGH, DATA MODE"

	; BPMSG1216
	.section .text.BPMSG1216, code
	.global _BPMSG1216_str
_BPMSG1216_str:
	.pascii "This mode requires an adapter"

	; BPMSG1219
	.section .text.BPMSG1219, code
	.global _BPMSG1219_str
_BPMSG1219_str:
	.pascii " 0.Macro menu\r\n 1.LCD Reset\r\n 2.Init LCD\r\n 3.Clear LCD\r\n 4.Cursor position ex:(4) 0\r\n 6.Write test numbers ex:(6) 80\r\n 7.Write test characters ex:(7) 80"

	; BPMSG1220
	.section .text.BPMSG1220, code
	.global _BPMSG1220_str
_BPMSG1220_str:
	.pascii "Display lines:\r\n 1. 1 \r\n 2. Multiple"

	; BPMSG1221
	.section .text.BPMSG1221, code
	.global _BPMSG1221_str
_BPMSG1221_str:
	.pascii "INIT"

	; BPMSG1222
	.section .text.BPMSG1222, code
	.global _BPMSG1222_str
_BPMSG1222_str:
	.pascii "CLEAR"

	; BPMSG1223
	.section .text.BPMSG1223, code
	.global _BPMSG1223_str
_BPMSG1223_str:
	.pascii "CURSOR SET"

	; BPMSG1226
	.section .text.BPMSG1226, code
	.global _BPMSG1226_str
_BPMSG1226_str:
	.pascii "Pinstates:"

	; BPMSG1228
	.section .text.BPMSG1228, code
	.global _BPMSG1228_str
_BPMSG1228_str:
	.pascii "P\tP\tP\tI\tI\t"

	; BPMSG1234
	.section .text.BPMSG1234, code
	.global _BPMSG1234_str
_BPMSG1234_str:
	.pascii "GND\t"

	; BPMSG1237
	.section .text.BPMSG1237, code
	.global _BPMSG1237_str
_BPMSG1237_str:
	.pascii " TIMEOUT"

	; BPMSG1238
	.section .text.BPMSG1238, code
	.global _BPMSG1238_str
_BPMSG1238_str:
	.pascii " 0. Macro menu\r\n 1. Live input monitor"

	; BPMSG1239
	.section .text.BPMSG1239, code
	.global _BPMSG1239_str
_BPMSG1239_str:
	.pascii "Input monitor, any key exits"

	; BPMSG1240
	.section .text.BPMSG1240, code
	.global _BPMSG1240_str
_BPMSG1240_str:
	.pascii " *startbit error"

	; BPMSG1241
	.section .text.BPMSG1241, code
	.global _BPMSG1241_str
_BPMSG1241_str:
	.pascii " *parity error"

	; BPMSG1242
	.section .text.BPMSG1242, code
	.global _BPMSG1242_str
_BPMSG1242_str:
	.pascii " *stopbit error"

	; BPMSG1243
	.section .text.BPMSG1243, code
	.global _BPMSG1243_str
_BPMSG1243_str:
	.pascii " NONE"

	; BPMSG1244
	.section .text.BPMSG1244, code
	.global _BPMSG1244_str
_BPMSG1244_str:
	.pascii " UNKNOWN ERROR"

	; BPMSG1245
	.section .text.BPMSG1245, code
	.global _BPMSG1245_str
_BPMSG1245_str:
	.pascii " autorange "

	; BPMSG1248
	.section .text.BPMSG1248, code
	.global _BPMSG1248_str
_BPMSG1248_str:
	.pascii "Input a custom BAUD rate:"

	; BPMSG1250
	.section .text.BPMSG1250, code
	.global _BPMSG1250_str
_BPMSG1250_str:
	.pascii "Any key to exit"

	; BPMSG1251
	.section .text.BPMSG1251, code
	.global _BPMSG1251_str
_BPMSG1251_str:
	.pascii "Space to continue"

	; BPMSG1252
	.section .text.BPMSG1252, code
	.global _BPMSG1252_str
_BPMSG1252_str:
	.pascii "Number of bits read/write: "

	; BPMSG1254
	.section .text.BPMSG1254, code
	.global _BPMSG1254_str
_BPMSG1254_str:
	.pascii "Position in degrees"

	; BPMSG1255
	.section .text.BPMSG1255, code
	.global _BPMSG1255_str
_BPMSG1255_str:
	.pascii "Servo active"

	; BPMSG1256
	.section .text.BPMSG1256, code
	.global _BPMSG1256_str
_BPMSG1256_str:
	.pascii "#12    \t#11    \t#10    \t#09   \t#08   \t#07   \t#06   \t#05   \t#04   \t#03   \t#02   \t#01   "

	; BPMSG1257
	.section .text.BPMSG1257, code
	.global _BPMSG1257_str
_BPMSG1257_str:
	.pascii "GND\t5.0V\t3.3V\tVPU\tADC\tAUX2\tAUX1\tAUX\t"

	; BPMSG1259
	.section .text.BPMSG1259, code
	.global _BPMSG1259_str
_BPMSG1259_str:
	.pascii "-\t-\t-\tOWD"

	; BPMSG1262
	.section .text.BPMSG1262, code
	.global _BPMSG1262_str
_BPMSG1262_str:
	.pascii "-\t-\tPGC\tPGD"

	; BPMSG1263
	.section .text.BPMSG1263, code
	.global _BPMSG1263_str
_BPMSG1263_str:
	.pascii "a/A/@ controls AUX1 pin"

	; BPMSG1264
	.section .text.BPMSG1264, code
	.global _BPMSG1264_str
_BPMSG1264_str:
	.pascii "a/A/@ controls AUX2 pin"

	; BPMSG1265
	.section .text.BPMSG1265, code
	.global _BPMSG1265_str
_BPMSG1265_str:
	.pascii "EEPROM"

	; BPMSG1266
	.section .text.BPMSG1266, code
	.global _BPMSG1266_str
_BPMSG1266_str:
	.pascii "SCL"

	; BPMSG1267
	.section .text.BPMSG1267, code
	.global _BPMSG1267_str
_BPMSG1267_str:
	.pascii "SDA"

	; BPMSG1268
	.section .text.BPMSG1268, code
	.global _BPMSG1268_str
_BPMSG1268_str:
	.pascii "WP"

	; BPMSG1269
	.section .text.BPMSG1269, code
	.global _BPMSG1269_str
_BPMSG1269_str:
	.pascii "READ&WRITE"

	; BPMSG1270
	.section .text.BPMSG1270, code
	.global _BPMSG1270_str
_BPMSG1270_str:
	.pascii "Vusb"

	; BPMSG1271
	.section .text.BPMSG1271, code
	.global _BPMSG1271_str
_BPMSG1271_str:
	.pascii "Select Vpu (Pullup) Source:\r\n 1) External (or None)\r\n 2) Onboard 3.3v\r\n 3) Onboard 5.0v"

	; BPMSG1272
	.section .text.BPMSG1272, code
	.global _BPMSG1272_str
_BPMSG1272_str:
	.pascii " on-board pullup voltage "

	; BPMSG1273
	.section .text.BPMSG1273, code
	.global _BPMSG1273_str
_BPMSG1273_str:
	.pascii "enabled"

	; BPMSG1274
	.section .text.BPMSG1274, code
	.global _BPMSG1274_str
_BPMSG1274_str:
	.pascii "disabled"

	; BPMSG1280
	.section .text.BPMSG1280, code
	.global _BPMSG1280_str
_BPMSG1280_str:
	.pascii "Waiting activity..."

	; BPMSG1281
	.section .text.BPMSG1281, code
	.global _BPMSG1281_str
_BPMSG1281_str:
	.pascii "** Early Exit!"

	; BPMSG1282
	.section .text.BPMSG1282, code
	.global _BPMSG1282_str
_BPMSG1282_str:
	.pascii "** Baud>16m: The BP cannot measure above 16000000, Done."

	; BPMSG1283
	.section .text.BPMSG1283, code
	.global _BPMSG1283_str
_BPMSG1283_str:
	.pascii "\n\rCalculated: \t"

	; BPMSG1284
	.section .text.BPMSG1284, code
	.global _BPMSG1284_str
_BPMSG1284_str:
	.pascii "\n\rEstimated:  \t"

	; BPMSG1285
	.section .text.BPMSG1285, code
	.global _BPMSG1285_str
_BPMSG1285_str:
	.pascii " bps"

	; HLP1000
	.section .text.HLP1000, code
	.global _HLP1000_str
_HLP1000_str:
	.pascii "General\t\t\t\t\tProtocol interaction"

	; HLP1001
	.section .text.HLP1001, code
	.global _HLP1001_str
_HLP1001_str:
	.pascii "---------------------------------------------------------------------------"

	; HLP1002
	.section .text.HLP1002, code
	.global _HLP1002_str
_HLP1002_str:
	.pascii "?\tThis help\t\t\t(0)\tList current macros"

	; HLP1003
	.section .text.HLP1003, code
	.global _HLP1003_str
_HLP1003_str:
	.pascii "=X/|X\tConverts X/reverse X\t\t(x)\tMacro x"

	; HLP1004
	.section .text.HLP1004, code
	.global _HLP1004_str
_HLP1004_str:
	.pascii "~\tSelftest\t\t\t[\tStart"

	; HLP1005
	.section .text.HLP1005, code
	.global _HLP1005_str
_HLP1005_str:
	.pascii "o\tSet output type\t\t\t]\tStop"

	; HLP1006
	.section .text.HLP1006, code
	.global _HLP1006_str
_HLP1006_str:
	.pascii "$\tJump to bootloader\t\t{\tStart with read"

	; HLP1007
	.section .text.HLP1007, code
	.global _HLP1007_str
_HLP1007_str:
	.pascii "&/%\tDelay 1 us/ms\t\t\t}\tStop"

	; HLP1008
	.section .text.HLP1008, code
	.global _HLP1008_str
_HLP1008_str:
	.pascii "a/A/@\tAUXPIN (low/HI/READ)\t\t\"abc\"\tSend string"

	; HLP1009
	.section .text.HLP1009, code
	.global _HLP1009_str
_HLP1009_str:
	.pascii "b\tSet baudrate\t\t\t123\tSend integer value"

	; HLP1010
	.section .text.HLP1010, code
	.global _HLP1010_str
_HLP1010_str:
	.pascii "c/C/k/K\tAUX assignment (A0/CS/A1/A2)\t0x123\tSend hex value"

	; HLP1011
	.section .text.HLP1011, code
	.global _HLP1011_str
_HLP1011_str:
	.pascii "d/D\tMeasure ADC (once/CONT.)\t0b110\tSend binary value"

	; HLP1012
	.section .text.HLP1012, code
	.global _HLP1012_str
_HLP1012_str:
	.pascii "f\tMeasure frequency\t\tr\tRead"

	; HLP1013
	.section .text.HLP1013, code
	.global _HLP1013_str
_HLP1013_str:
	.pascii "g/S\tGenerate PWM/Servo\t\t/\tCLK hi"

	; HLP1014
	.section .text.HLP1014, code
	.global _HLP1014_str
_HLP1014_str:
	.pascii "h\tCommandhistory\t\t\t\\\tCLK lo"

	; HLP1015
	.section .text.HLP1015, code
	.global _HLP1015_str
_HLP1015_str:
	.pascii "i\tVersioninfo/statusinfo\t\t^\tCLK tick"

	; HLP1016
	.section .text.HLP1016, code
	.global _HLP1016_str
_HLP1016_str:
	.pascii "l/L\tBitorder (msb/LSB)\t\t-\tDAT hi"

	; HLP1017
	.section .text.HLP1017, code
	.global _HLP1017_str
_HLP1017_str:
	.pascii "m\tChange mode\t\t\t_\tDAT lo"

	; HLP1018
	.section .text.HLP1018, code
	.global _HLP1018_str
_HLP1018_str:
	.pascii "e\tSet Pullup Method\t\t.\tDAT read"

	; HLP1019
	.section .text.HLP1019, code
	.global _HLP1019_str
_HLP1019_str:
	.pascii "p/P\tPullup resistors (off/ON)\t!\tBit read"

	; HLP1020
	.section .text.HLP1020, code
	.global _HLP1020_str
_HLP1020_str:
	.pascii "s\tScript engine\t\t\t:\tRepeat e.g. r:10"

	; HLP1021
	.section .text.HLP1021, code
	.global _HLP1021_str
_HLP1021_str:
	.pascii "v\tShow volts/states\t\t;\tBits to read/write e.g. 0x55;2"

	; HLP1022
	.section .text.HLP1022, code
	.global _HLP1022_str
_HLP1022_str:
	.pascii "w/W\tPSU (off/ON)\t\t<x>/<x= >/<0>\tUsermacro x/assign x/list all"

	; MSG_1WIRE_MODE_IDENTIFIER
	.section .text.MSG_1WIRE_MODE_IDENTIFIER, code
	.global _MSG_1WIRE_MODE_IDENTIFIER_str
_MSG_1WIRE_MODE_IDENTIFIER_str:
	.pascii "1W01"

	; MSG_1WIRE_SPEED_PROMPT
	.section .text.MSG_1WIRE_SPEED_PROMPT, code
	.global _MSG_1WIRE_SPEED_PROMPT_str
_MSG_1WIRE_SPEED_PROMPT_str:
	.pascii "Set speed:\r\n 1. Standard (~16.3kbps) \r\n 2. Overdrive (~160kps)"

	; MSG_ACK
	.section .text.MSG_ACK, code
	.global _MSG_ACK_str
_MSG_ACK_str:
	.pascii "ACK"

	; MSG_BASE_CONVERTER_EQUAL_SIGN
	.section .text.MSG_BASE_CONVERTER_EQUAL_SIGN, code
	.global _MSG_BASE_CONVERTER_EQUAL_SIGN_str
_MSG_BASE_CONVERTER_EQUAL_SIGN_str:
	.pascii " = "

	; MSG_BAUD_DETECTION_SELECTED
	.section .text.MSG_BAUD_DETECTION_SELECTED, code
	.global _MSG_BAUD_DETECTION_SELECTED_str
_MSG_BAUD_DETECTION_SELECTED_str:
	.pascii "Baud detection selected.."

	; MSG_BBIO_MODE_IDENTIFIER
	.section .text.MSG_BBIO_MODE_IDENTIFIER, code
	.global _MSG_BBIO_MODE_IDENTIFIER_str
_MSG_BBIO_MODE_IDENTIFIER_str:
	.pascii "BBIO1"

	; MSG_BINARY_NUMBER_PREFIX
	.section .text.MSG_BINARY_NUMBER_PREFIX, code
	.global _MSG_BINARY_NUMBER_PREFIX_str
_MSG_BINARY_NUMBER_PREFIX_str:
	.pascii "0b"

	; MSG_CFG0_FIELD
	.section .text.MSG_CFG0_FIELD, code
	.global _MSG_CFG0_FIELD_str
_MSG_CFG0_FIELD_str:
	.pascii "CFG0: "

	; MSG_CHIP_REVISION_A3
	.section .text.MSG_CHIP_REVISION_A3, code
	.global _MSG_CHIP_REVISION_A3_str
_MSG_CHIP_REVISION_A3_str:
	.pascii "A3"

	; MSG_CHIP_REVISION_A5
	.section .text.MSG_CHIP_REVISION_A5, code
	.global _MSG_CHIP_REVISION_A5_str
_MSG_CHIP_REVISION_A5_str:
	.pascii "A5"

	; MSG_CHIP_REVISION_ID_BEGIN
	.section .text.MSG_CHIP_REVISION_ID_BEGIN, code
	.global _MSG_CHIP_REVISION_ID_BEGIN_str
_MSG_CHIP_REVISION_ID_BEGIN_str:
	.pascii " (24FJ256GB106 "

	; MSG_CHIP_REVISION_UNKNOWN
	.section .text.MSG_CHIP_REVISION_UNKNOWN, code
	.global _MSG_CHIP_REVISION_UNKNOWN_str
_MSG_CHIP_REVISION_UNKNOWN_str:
	.pascii "UNK"

	; MSG_CLUTCH_DISENGAGED
	.section .text.MSG_CLUTCH_DISENGAGED, code
	.global _MSG_CLUTCH_DISENGAGED_str
_MSG_CLUTCH_DISENGAGED_str:
	.pascii "Clutch disengaged!!!"

	; MSG_CLUTCH_ENGAGED
	.section .text.MSG_CLUTCH_ENGAGED, code
	.global _MSG_CLUTCH_ENGAGED_str
_MSG_CLUTCH_ENGAGED_str:
	.pascii "Clutch engaged!!!"

	; MSG_FINISH_SETUP_PROMPT
	.section .text.MSG_FINISH_SETUP_PROMPT, code
	.global _MSG_FINISH_SETUP_PROMPT_str
_MSG_FINISH_SETUP_PROMPT_str:
	.pascii "To finish setup, start up the power supplies with command 'W'"

	; MSG_HEXADECIMAL_NUMBER_PREFIX
	.section .text.MSG_HEXADECIMAL_NUMBER_PREFIX, code
	.global _MSG_HEXADECIMAL_NUMBER_PREFIX_str
_MSG_HEXADECIMAL_NUMBER_PREFIX_str:
	.pascii "0x"

	; MSG_I2C_MODE_IDENTIFIER
	.section .text.MSG_I2C_MODE_IDENTIFIER, code
	.global _MSG_I2C_MODE_IDENTIFIER_str
_MSG_I2C_MODE_IDENTIFIER_str:
	.pascii "I2C1"

	; MSG_I2C_PINS_STATE
	.section .text.MSG_I2C_PINS_STATE, code
	.global _MSG_I2C_PINS_STATE_str
_MSG_I2C_PINS_STATE_str:
	.pascii "-\t-\tSCL\tSDA"

	; MSG_I2C_READ_ADDRESS_END
	.section .text.MSG_I2C_READ_ADDRESS_END, code
	.global _MSG_I2C_READ_ADDRESS_END_str
_MSG_I2C_READ_ADDRESS_END_str:
	.pascii " R) "

	; MSG_I2C_START_BIT
	.section .text.MSG_I2C_START_BIT, code
	.global _MSG_I2C_START_BIT_str
_MSG_I2C_START_BIT_str:
	.pascii "I2C START BIT"

	; MSG_I2C_STOP_BIT
	.section .text.MSG_I2C_STOP_BIT, code
	.global _MSG_I2C_STOP_BIT_str
_MSG_I2C_STOP_BIT_str:
	.pascii "I2C STOP BIT"

	; MSG_I2C_WRITE_ADDRESS_END
	.section .text.MSG_I2C_WRITE_ADDRESS_END, code
	.global _MSG_I2C_WRITE_ADDRESS_END_str
_MSG_I2C_WRITE_ADDRESS_END_str:
	.pascii " W) "

	; MSG_MODE_HEADER_END
	.section .text.MSG_MODE_HEADER_END, code
	.global _MSG_MODE_HEADER_END_str
_MSG_MODE_HEADER_END_str:
	.pascii " )"

	; MSG_NACK
	.section .text.MSG_NACK, code
	.global _MSG_NACK_str
_MSG_NACK_str:
	.pascii "NACK"

	; MSG_NO_VOLTAGE_ON_PULLUP_PIN
	.section .text.MSG_NO_VOLTAGE_ON_PULLUP_PIN, code
	.global _MSG_NO_VOLTAGE_ON_PULLUP_PIN_str
_MSG_NO_VOLTAGE_ON_PULLUP_PIN_str:
	.pascii "Warning: no voltage on Vpullup pin"

	; MSG_ONBOARD_I2C_EEPROM_WRITE_PROTECT_DISABLED
	.section .text.MSG_ONBOARD_I2C_EEPROM_WRITE_PROTECT_DISABLED, code
	.global _MSG_ONBOARD_I2C_EEPROM_WRITE_PROTECT_DISABLED_str
_MSG_ONBOARD_I2C_EEPROM_WRITE_PROTECT_DISABLED_str:
	.pascii "On-board EEPROM write protect disabled"

	; MSG_PIC_MODE_IDENTIFIER
	.section .text.MSG_PIC_MODE_IDENTIFIER, code
	.global _MSG_PIC_MODE_IDENTIFIER_str
_MSG_PIC_MODE_IDENTIFIER_str:
	.pascii "PIC1"

	; MSG_PIC_UNKNOWN_MODE
	.section .text.MSG_PIC_UNKNOWN_MODE, code
	.global _MSG_PIC_UNKNOWN_MODE_str
_MSG_PIC_UNKNOWN_MODE_str:
	.pascii "unknown mode"

	; MSG_PIN_OUTPUT_TYPE_PROMPT
	.section .text.MSG_PIN_OUTPUT_TYPE_PROMPT, code
	.global _MSG_PIN_OUTPUT_TYPE_PROMPT_str
_MSG_PIN_OUTPUT_TYPE_PROMPT_str:
	.pascii "Select output type:\r\n 1. Open drain (H=Hi-Z, L=GND)\r\n 2. Normal (H=3.3V, L=GND)"

	; MSG_PWM_FREQUENCY_TOO_LOW
	.section .text.MSG_PWM_FREQUENCY_TOO_LOW, code
	.global _MSG_PWM_FREQUENCY_TOO_LOW_str
_MSG_PWM_FREQUENCY_TOO_LOW_str:
	.pascii "Frequencies < 1Hz are not supported."

	; MSG_PWM_HZ_MARKER
	.section .text.MSG_PWM_HZ_MARKER, code
	.global _MSG_PWM_HZ_MARKER_str
_MSG_PWM_HZ_MARKER_str:
	.pascii " Hz"

	; MSG_RAW2WIRE_ATR_DATA_UNITS_HEADER
	.section .text.MSG_RAW2WIRE_ATR_DATA_UNITS_HEADER, code
	.global _MSG_RAW2WIRE_ATR_DATA_UNITS_HEADER_str
_MSG_RAW2WIRE_ATR_DATA_UNITS_HEADER_str:
	.pascii "Data units: "

	; MSG_RAW2WIRE_ATR_DATA_UNITS_NO_INDICATION
	.section .text.MSG_RAW2WIRE_ATR_DATA_UNITS_NO_INDICATION, code
	.global _MSG_RAW2WIRE_ATR_DATA_UNITS_NO_INDICATION_str
_MSG_RAW2WIRE_ATR_DATA_UNITS_NO_INDICATION_str:
	.pascii "no indication"

	; MSG_RAW2WIRE_ATR_DATA_UNIT_LENGTH
	.section .text.MSG_RAW2WIRE_ATR_DATA_UNIT_LENGTH, code
	.global _MSG_RAW2WIRE_ATR_DATA_UNIT_LENGTH_str
_MSG_RAW2WIRE_ATR_DATA_UNIT_LENGTH_str:
	.pascii "Data unit length (bits): "

	; MSG_RAW2WIRE_ATR_PROTOCOL_2WIRE
	.section .text.MSG_RAW2WIRE_ATR_PROTOCOL_2WIRE, code
	.global _MSG_RAW2WIRE_ATR_PROTOCOL_2WIRE_str
_MSG_RAW2WIRE_ATR_PROTOCOL_2WIRE_str:
	.pascii "2 wire"

	; MSG_RAW2WIRE_ATR_PROTOCOL_3WIRE
	.section .text.MSG_RAW2WIRE_ATR_PROTOCOL_3WIRE, code
	.global _MSG_RAW2WIRE_ATR_PROTOCOL_3WIRE_str
_MSG_RAW2WIRE_ATR_PROTOCOL_3WIRE_str:
	.pascii "3 wire"

	; MSG_RAW2WIRE_ATR_PROTOCOL_HEADER
	.section .text.MSG_RAW2WIRE_ATR_PROTOCOL_HEADER, code
	.global _MSG_RAW2WIRE_ATR_PROTOCOL_HEADER_str
_MSG_RAW2WIRE_ATR_PROTOCOL_HEADER_str:
	.pascii "Protocol: "

	; MSG_RAW2WIRE_ATR_PROTOCOL_SERIAL
	.section .text.MSG_RAW2WIRE_ATR_PROTOCOL_SERIAL, code
	.global _MSG_RAW2WIRE_ATR_PROTOCOL_SERIAL_str
_MSG_RAW2WIRE_ATR_PROTOCOL_SERIAL_str:
	.pascii "serial"

	; MSG_RAW2WIRE_ATR_PROTOCOL_UNKNOWN
	.section .text.MSG_RAW2WIRE_ATR_PROTOCOL_UNKNOWN, code
	.global _MSG_RAW2WIRE_ATR_PROTOCOL_UNKNOWN_str
_MSG_RAW2WIRE_ATR_PROTOCOL_UNKNOWN_str:
	.pascii "unknown"

	; MSG_RAW2WIRE_ATR_READ_TYPE_HEADER
	.section .text.MSG_RAW2WIRE_ATR_READ_TYPE_HEADER, code
	.global _MSG_RAW2WIRE_ATR_READ_TYPE_HEADER_str
_MSG_RAW2WIRE_ATR_READ_TYPE_HEADER_str:
	.pascii "Read type: "

	; MSG_RAW2WIRE_ATR_READ_TYPE_TO_END
	.section .text.MSG_RAW2WIRE_ATR_READ_TYPE_TO_END, code
	.global _MSG_RAW2WIRE_ATR_READ_TYPE_TO_END_str
_MSG_RAW2WIRE_ATR_READ_TYPE_TO_END_str:
	.pascii "to end"

	; MSG_RAW2WIRE_ATR_READ_TYPE_VARIABLE_LENGTH
	.section .text.MSG_RAW2WIRE_ATR_READ_TYPE_VARIABLE_LENGTH, code
	.global _MSG_RAW2WIRE_ATR_READ_TYPE_VARIABLE_LENGTH_str
_MSG_RAW2WIRE_ATR_READ_TYPE_VARIABLE_LENGTH_str:
	.pascii "variable length"

	; MSG_RAW2WIRE_ATR_REPLY_HEADER
	.section .text.MSG_RAW2WIRE_ATR_REPLY_HEADER, code
	.global _MSG_RAW2WIRE_ATR_REPLY_HEADER_str
_MSG_RAW2WIRE_ATR_REPLY_HEADER_str:
	.pascii "ISO 7816-3 reply (uses current LSB setting): "

	; MSG_RAW2WIRE_ATR_RFU
	.section .text.MSG_RAW2WIRE_ATR_RFU, code
	.global _MSG_RAW2WIRE_ATR_RFU_str
_MSG_RAW2WIRE_ATR_RFU_str:
	.pascii "RFU"

	; MSG_RAW2WIRE_ATR_TRIGGER_INFO
	.section .text.MSG_RAW2WIRE_ATR_TRIGGER_INFO, code
	.global _MSG_RAW2WIRE_ATR_TRIGGER_INFO_str
_MSG_RAW2WIRE_ATR_TRIGGER_INFO_str:
	.pascii "ISO 7816-3 ATR (RESET on CS)\r\nRESET HIGH, CLOCK TICK, RESET LOW"

	; MSG_RAW2WIRE_I2C_START
	.section .text.MSG_RAW2WIRE_I2C_START, code
	.global _MSG_RAW2WIRE_I2C_START_str
_MSG_RAW2WIRE_I2C_START_str:
	.pascii "(\\-/_\\-)"

	; MSG_RAW2WIRE_I2C_STOP
	.section .text.MSG_RAW2WIRE_I2C_STOP, code
	.global _MSG_RAW2WIRE_I2C_STOP_str
_MSG_RAW2WIRE_I2C_STOP_str:
	.pascii "(\\_/-)"

	; MSG_RAW2WIRE_MACRO_MENU
	.section .text.MSG_RAW2WIRE_MACRO_MENU, code
	.global _MSG_RAW2WIRE_MACRO_MENU_str
_MSG_RAW2WIRE_MACRO_MENU_str:
	.pascii " 0.Macro menu\r\n 1.ISO7816-3 ATR\r\n 2.ISO7816-3 parse only"

	; MSG_RAW2WIRE_MODE_HEADER
	.section .text.MSG_RAW2WIRE_MODE_HEADER, code
	.global _MSG_RAW2WIRE_MODE_HEADER_str
_MSG_RAW2WIRE_MODE_HEADER_str:
	.pascii "R2W (spd hiz)=( "

	; MSG_RAW3WIRE_MODE_HEADER
	.section .text.MSG_RAW3WIRE_MODE_HEADER, code
	.global _MSG_RAW3WIRE_MODE_HEADER_str
_MSG_RAW3WIRE_MODE_HEADER_str:
	.pascii "R3W (spd csl hiz)=( "

	; MSG_RAW_BRG_VALUE_INPUT
	.section .text.MSG_RAW_BRG_VALUE_INPUT, code
	.global _MSG_RAW_BRG_VALUE_INPUT_str
_MSG_RAW_BRG_VALUE_INPUT_str:
	.pascii "Enter raw value for BRG"

	; MSG_RAW_MODE_IDENTIFIER
	.section .text.MSG_RAW_MODE_IDENTIFIER, code
	.global _MSG_RAW_MODE_IDENTIFIER_str
_MSG_RAW_MODE_IDENTIFIER_str:
	.pascii "RAW1"

	; MSG_RESET_MESSAGE
	.section .text.MSG_RESET_MESSAGE, code
	.global _MSG_RESET_MESSAGE_str
_MSG_RESET_MESSAGE_str:
	.pascii "RESET"

	; MSG_SOFTWARE_MODE_SPEED_PROMPT
	.section .text.MSG_SOFTWARE_MODE_SPEED_PROMPT, code
	.global _MSG_SOFTWARE_MODE_SPEED_PROMPT_str
_MSG_SOFTWARE_MODE_SPEED_PROMPT_str:
	.pascii "Set speed:\r\n 1. ~5KHz\r\n 2. ~50KHz\r\n 3. ~100KHz\r\n 4. ~400KHz"

	; MSG_SPI_COULD_NOT_KEEP_UP
	.section .text.MSG_SPI_COULD_NOT_KEEP_UP, code
	.global _MSG_SPI_COULD_NOT_KEEP_UP_str
_MSG_SPI_COULD_NOT_KEEP_UP_str:
	.pascii "Couldn't keep up"

	; MSG_SPI_CS_DISABLED
	.section .text.MSG_SPI_CS_DISABLED, code
	.global _MSG_SPI_CS_DISABLED_str
_MSG_SPI_CS_DISABLED_str:
	.pascii "CS DISABLED"

	; MSG_SPI_CS_ENABLED
	.section .text.MSG_SPI_CS_ENABLED, code
	.global _MSG_SPI_CS_ENABLED_str
_MSG_SPI_CS_ENABLED_str:
	.pascii "CS ENABLED"

	; MSG_SPI_CS_MODE_PROMPT
	.section .text.MSG_SPI_CS_MODE_PROMPT, code
	.global _MSG_SPI_CS_MODE_PROMPT_str
_MSG_SPI_CS_MODE_PROMPT_str:
	.pascii "CS:\r\n 1. CS\r\n 2. /CS *default"

	; MSG_SPI_MODE_IDENTIFIER
	.section .text.MSG_SPI_MODE_IDENTIFIER, code
	.global _MSG_SPI_MODE_IDENTIFIER_str
_MSG_SPI_MODE_IDENTIFIER_str:
	.pascii "SPI1"

	; MSG_SPI_PINS_STATE
	.section .text.MSG_SPI_PINS_STATE, code
	.global _MSG_SPI_PINS_STATE_str
_MSG_SPI_PINS_STATE_str:
	.pascii "CS\tMISO\tCLK\tMOSI"

	; MSG_UART_MODE_IDENTIFIER
	.section .text.MSG_UART_MODE_IDENTIFIER, code
	.global _MSG_UART_MODE_IDENTIFIER_str
_MSG_UART_MODE_IDENTIFIER_str:
	.pascii "ART1"

	; MSG_UART_NORMAL_TO_EXIT
	.section .text.MSG_UART_NORMAL_TO_EXIT, code
	.global _MSG_UART_NORMAL_TO_EXIT_str
_MSG_UART_NORMAL_TO_EXIT_str:
	.pascii "Normal to exit"

	; MSG_UART_PINS_STATE
	.section .text.MSG_UART_PINS_STATE, code
	.global _MSG_UART_PINS_STATE_str
_MSG_UART_PINS_STATE_str:
	.pascii "-\tRxD\t-\tTxD"

	; MSG_UNKNOWN_MACRO_ERROR
	.section .text.MSG_UNKNOWN_MACRO_ERROR, code
	.global _MSG_UNKNOWN_MACRO_ERROR_str
_MSG_UNKNOWN_MACRO_ERROR_str:
	.pascii "Unknown macro, try ? or (0) for help"

	; MSG_USING_ONBOARD_I2C_EEPROM
	.section .text.MSG_USING_ONBOARD_I2C_EEPROM, code
	.global _MSG_USING_ONBOARD_I2C_EEPROM_str
_MSG_USING_ONBOARD_I2C_EEPROM_str:
	.pascii "Now using on-board EEPROM I2C interface"

	; MSG_VOLTAGE_VPULLUP_ALREADY_PRESENT
	.section .text.MSG_VOLTAGE_VPULLUP_ALREADY_PRESENT, code
	.global _MSG_VOLTAGE_VPULLUP_ALREADY_PRESENT_str
_MSG_VOLTAGE_VPULLUP_ALREADY_PRESENT_str:
	.pascii "Warning: already a voltage on Vpullup pin"

	; MSG_VPU_3V3_MARKER
	.section .text.MSG_VPU_3V3_MARKER, code
	.global _MSG_VPU_3V3_MARKER_str
_MSG_VPU_3V3_MARKER_str:
	.pascii "Vpu=3V3, "

	; MSG_VPU_5V_MARKER
	.section .text.MSG_VPU_5V_MARKER, code
	.global _MSG_VPU_5V_MARKER_str
_MSG_VPU_5V_MARKER_str:
	.pascii "Vpu=5V, "

	; MSG_VREG_TOO_LOW
	.section .text.MSG_VREG_TOO_LOW, code
	.global _MSG_VREG_TOO_LOW_str
_MSG_VREG_TOO_LOW_str:
	.pascii "VREG too low, is there a short?"

	; MSG_XSV1_MODE_IDENTIFIER
	.section .text.MSG_XSV1_MODE_IDENTIFIER, code
	.global _MSG_XSV1_MODE_IDENTIFIER_str
_MSG_XSV1_MODE_IDENTIFIER_str:
	.pascii "XSV1"

