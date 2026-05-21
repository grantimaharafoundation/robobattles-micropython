# Set up controller
controller = XboxController()

# Set up motors
left = Motor(Port.A)
right = Motor(Port.B)
weapon = Motor(Port.C)

# Run program loop
while True:
    # Get input
    compX = controller.joystick_left()[0]
    compY = controller.joystick_left()[1]
    compWL = controller.triggers()[0]
    compWR = controller.triggers()[1]

    # Accept D-Pad for drive motor control to support adaptive controllers
    if Button.LEFT in controller.buttons.pressed():
        compX -= 100

    if Button.RIGHT in controller.buttons.pressed():
        compX += 100

    if Button.UP in controller.buttons.pressed():
        compY += 100

    if Button.DOWN in controller.buttons.pressed():
        compY += 100

    # Accept buttons for weapon motor control to support adaptive controller
    if Button.B in controller.buttons.pressed():
        compWR += 100

    if Button.A in controller.buttons.pressed():
        compWL += 100

    # Reduce left/right input for slower turning to make bot easier to control
    compX /= 3

    # Control weapon motors
    if compWL - compWR != 0:
        weapon.go(compWL - compWR)
    else:
        # Triggers are not pressed, so let weapon motor spin freely
        weapon.stop()

    # Control wheel motors
    if compX != 0 or compY != 0:
        # Joystick is pressed, so move wheel motors
        left.go(-compY - compX)
        right.go(compY - compX)
    else:
        # Joystick is not pressed, so let wheel motors spin freely
        left.stop()
        right.stop()
