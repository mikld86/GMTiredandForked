legend = [
    ("Ground", "gnd"),
    ("Power", "pwr"),
    ("PWM", "pwm"),
    ("AC", "ac"),
    ("Doc", "doc")
]

# Pinlabels

left_header = [
    [
        ("VCC", "pwr"),
        ("HE1 24V", "doc"),
    ],
    [
        ("GND", "gnd"),
        ("HE1 GND", "doc"),
    ],
    [
        ("PWM", "pwm"),
        ("LED PWM", "doc"),
    ],
]

lower_header = [
    [
        ("GND", "gnd"),
        ("4010 FAN", "doc"),
    ],
    [
        ("VCC", "pwr"),
        ("4010 FAN", "doc"),
    ],
]

ac_in = [
    [
        ("N", "ac"),
    ],
    [
        ("L", "ac"),
    ],
]

relais_out = [
    [
        ("Grinder", "ac"),
        ("gpio11", "doc"),
    ],
    [
        ("Valve", "ac"),
        ("gpio10", "doc"),
    ],
    [
        ("Pump", "ac"),
        ("gpio9", "doc"),
    ],
]

annotation_usb = ["USB-C", "port"]
annotation_btn_boot = ["BOOT", "button"]
annotation_btn_reset = ["EN", "button"]