# Set up controller
controller = XboxController()

# Set up motors
left = Motor(Port.A)
right = Motor(Port.B)
weapon = Motor(Port.C)

# Apply joystick deadzone to prevent drift
def apply_deadzone(value):
    # Set joystick deadzone
    deadzone = 20

    # Input falls within deadzone, so ignore
    if abs(value) < deadzone:
        return 0

    # Scale so that deadzone → 0 and max → 1
    sign = 1 if value > 0 else -1
    scaled = (abs(value) - deadzone) / (1 - deadzone / 100)
    return scaled * sign


# Run program loop
while True:
    # Get input and apply deadzone math
    compX = apply_deadzone(controller.joystick_left()[0])
    compY = apply_deadzone(controller.joystick_left()[1])
    compWL = apply_deadzone(controller.triggers()[0])
    compWR = apply_deadzone(controller.triggers()[1])

    # Reduce left/right input for slower turning to make bot easier to control
    compX /= 3

    # Control weapon motors
    weapon.go(compWL - compWR)

    # Control wheel motors
    if compX != 0 or compY != 0:
        # Joystick is pressed, so move wheel motors
        left.go(-compY - compX)
        right.go(compY - compX)
    else:
        # Joystick is not pressed, so let wheel motors spin freely to save power
        left.stop()
        right.stop()
