from pinout.core import Group, Image
from pinout.components.layout import Diagram
from pinout.components.pinlabel import PinLabelGroup, PinLabel, Body
from pinout.components.text import TextBlock
from pinout.components.annotation import AnnotationLabel
from pinout.components import leaderline as lline
from pinout.components.legend import Legend


# Import data for the diagram
import data

# Create a new diagram
diagram = Diagram(1600, 1100, "diagram")

# Add a stylesheet
diagram.add_stylesheet("styles_auto.css", True)
diagram.add_stylesheet("styles.css", True)

# Create a group to hold the pinout-diagram components.
graphic = diagram.add(Group(169, 42))

# Add and embed an image
hardware = graphic.add(Image("../docs/assets/pcb_top.png", embed=True))

# Measure and record key locations with the hardware Image instance
hardware.add_coord("in_vcc", 324, 54)
hardware.add_coord("ac_in", 1015, 65)
hardware.add_coord("relais_out", 1015, 205)
hardware.add_coord("fan_gnd", 340, 630)
# Other (x,y) pairs can also be stored here
hardware.add_coord("pin_pitch_v", 0, 30)
hardware.add_coord("pin_pitch_v_ac", 0, 60)
hardware.add_coord("pin_pitch_h", 30, 0)

# Create pinlabels on the left header
graphic.add(
    PinLabelGroup(
        x=hardware.coord("in_vcc").x,
        y=hardware.coord("in_vcc").y,
        pin_pitch=hardware.coord("pin_pitch_h", raw=True),
        label_start=(284, 10),
        label_pitch=(0, 30),
        scale=(-1, -1),
        labels=data.left_header,
        leaderline=lline.Curved(direction="vh"),
    )
)

# Create pinlabels on the lower header
graphic.add(
    PinLabelGroup(
        x=hardware.coord("fan_gnd").x,
        y=hardware.coord("fan_gnd").y,
        scale=(-1, 1),
        pin_pitch=hardware.coord("pin_pitch_h", raw=True),
        label_start=(300, 10),
        label_pitch=(0, 30),
        labels=data.lower_header,
        leaderline=lline.Curved(direction="vh"),
    )
)

# Create pinlabels on the right header
graphic.add(
    PinLabelGroup(
        x=hardware.coord("ac_in").x,
        y=hardware.coord("ac_in").y,
        pin_pitch=hardware.coord("pin_pitch_v_ac", raw=True),
        label_start=(60, 0),
        label_pitch=hardware.coord("pin_pitch_v_ac", raw=True),
        labels=data.ac_in,
    )
)
graphic.add(
    PinLabelGroup(
        x=hardware.coord("relais_out").x,
        y=hardware.coord("relais_out").y,
        pin_pitch=hardware.coord("pin_pitch_v_ac", raw=True),
        label_start=(60, 0),
        label_pitch=hardware.coord("pin_pitch_v_ac", raw=True),
        labels=data.relais_out,
    )
)

graphic.add(
    AnnotationLabel(
        x=410,
        y=760,
        scale=(-1, 1),
        content={"x": 102, "y": 76, "content": data.annotation_usb},
        body={"y": 58, "width": 125},
        target={"x": -20, "y": -20, "width": 40, "height": 40, "corner_radius": 20},
    )
)
graphic.add(
    AnnotationLabel(
        x=515,
        y=750,
        scale=(-1, 1),
        content={"x": 204, "y": 146, "content": data.annotation_btn_reset},
        body={"x": 142, "y": 128, "width": 125},
        target={"x": -20, "y": -20, "width": 40, "height": 40, "corner_radius": 20},
    )
)
graphic.add(
    AnnotationLabel(
        x=587,
        y=750,
        scale=(-1, 1),
        content={"x": 279, "y": 206, "content": data.annotation_btn_boot},
        body={"x": 217, "y": 188, "width": 125},
        target={"x": -20, "y": -20, "width": 40, "height": 40, "corner_radius": 20},
    )
)
