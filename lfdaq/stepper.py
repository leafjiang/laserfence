"""
A class to make working with the stepper motor easier.
"""

UE9_WIRING_DESCRIPTION = """
Connection on DCA-10 -> Connection on LabJack
IN1 -> FIO1
IN2 -> FIO0
EN  -> FIO2
GND -> GND
CS  -> AIN0
"""

UE9_WITH_ENCODER_WIRING_DESCRIPTION = """
Connection on DCA-10 -> Connection on LabJack
IN1      -> EIO1
IN2      -> FIO2
EN       -> EIO0
GND      -> GND
CS       -> AIN0

Encoder1 -> FIO0
Encoder2 -> FIO2
Power    -> VS
GND      -> GND
"""

class StepperException(Exception): pass

class NoDevicesConnectedException(StepperException): pass

class InvalidConfigurationException(StepperException): pass

class Stepper(object):
    """
    The Stepper Class provides a nice interface for working with the
    stepper motor controller and a LabJack.
    """
    def __init__(self, device, encoderAttached = False):
        """
        Name: Stepper.__init__(device, encoderAttached = False)

        Args: devType, set to ue9.UE9() device handle
              encoderAttached, set to True if your motor has a quadrature
              encoder output and you want to use it. Note: This requires the
              use of the EIOs so a CB15 is required.

        Desc: Makes a new instance of the Stepper class.

        Examples:

        To open the first found device, without an encoder:
        >>> from stepper import Stepper
        >>> s = Stepper(d)

        To open the first found UE9, with an encoder:
        >>> from stepper import Stepper
        >>> s = Stepper(d, encoderAttached = True)
        >>> d = stepper.Stepper(devType = 6, encoderAttached = True)
        """

        self.device = device
        self.encoderAttached = encoderAttached
        self.directionLine = None
        self.enableLine = None
        self.currentLine = None

        self.enableState = 1
        self.directionState = 1

        if device is None:
           raise InvalidConfigurationException("Invalid device submitted.")

        if self.encoderAttached:
            self.directionLine = 9
            self.enableLine = 8
        else:
            self.directionLine = 1
            self.enableLine = 2
        self.currentLine = 0

        # Make sure all the pins are digital, and enable a timer.
        self.device.writeRegister(7000, 1)
        self.device.writeRegister(7002, 1)

        # Set the Timer for PWM and Duty Cycle of 0%
        if self.encoderAttached:
            self.device.writeRegister(50501, 3)
            self.device.writeRegister(7100, [8, 0, 8, 0, 0, 65535])
        else:
            self.device.writeRegister(50500, 0)
            self.device.writeRegister(50501, 1)
            self.device.writeRegister(7100, [0, 65535])

        # Set the direction and enable lines to output.
        # Don't have to do this because modbus will take care of direction.
        #self.device.writeRegister(6100 + self.enableLine, 1)
        #self.device.writeRegister(6100 + self.directionLine, 1)

        # Set the direction and enable lines high.
        self.device.writeRegister(6000 + self.enableLine, 1)
        self.device.writeRegister(6000 + self.directionLine, 1)

    def startMotor(self, dutyCycle = 1):
        """
        Name: Stepper.startMotor(dutyCycle = 1)

        Args: dutyCycle, a value between 0-1 representing the duty cycle of the
              PWM output. ( Controls how fast the motor spins ).

        Desc: Starts the motor at the specified duty cycle.
              By default, will start the motor with a 100% duty cycle.

        Example:
        >>> from stepper import Stepper
        >>> d = stepper.Stepper()
        >>> d.startMotor(dutyCycle = 0.5)
        """
        if dutyCycle < 0 or dutyCycle > 1:
            raise InvalidConfigurationException("Duty cycle must be between 0 and 1. Got %s." % dutyCycle)

        value = int(65535 - (65535 * dutyCycle))
        if self.encoderAttached:
            self.device.writeRegister(7104, [0, value])
        else:
            self.device.writeRegister(7100, [0, value])

        if not self.enableState:
            self.device.writeRegister(6000 + self.enableLine, 1)

    def stopMotor(self):
        """
        Name: Stepper.stopMotor()

        Args: None

        Desc: Sets the enable line low, stopping the motor.

        Example:
        >>> from stepper import Stepper
        >>> d = stepper.Stepper()
        >>> d.startMotor(dutyCycle = 0.5)
        >>> d.stopMotor()
        """
        self.device.writeRegister(6000 + self.enableLine, 0)
        self.enableState = 0

    def toggleDirection(self):
        """
        Name: Stepper.toggleDirection()

        Args: None

        Desc: Toggles the direction line, which causes the motor to change
              directions.

        Example:
        >>> from stepper import Stepper
        >>> d = stepper.Stepper()
        >>> d.startMotor(dutyCycle = 0.5)
        >>> d.toggleDirection()
        """
        self.directionState = not self.directionState
        self.device.writeRegister(6000 + self.directionLine, self.directionState)

    def readCurrent(self):
        """
        Name: Stepper.readCurrent()

        Args: None

        Desc: Takes a sample off the CS line and applies the offset from the
              DCA-10 datasheet. Returns a floating point value representing
              Amps.

        Example:
        >>> from stepper import Stepper
        >>> d = stepper.Stepper()
        >>> d.startMotor(dutyCycle = 0.5)
        >>> print d.readCurrent()
        0.018158430290222165
        """
        return (self.device.readRegister(0 + self.currentLine) * 3.7596 )

    def readEncoder(self):
        """
        Name: Stepper.readEncoder()

        Args: None

        Desc: Reads the current value of the quadrature encoder. Raises a
              StepperException if the encoder is not attached.

        Example:
        >>> from stepper import Stepper
        >>> d = stepper.Stepper(encoderAttached = True)
        >>> d.startMotor(dutyCycle = 0.5)
        >>> print d.readEncoder()
        3925
        """
        if self.encoderAttached:
            return self.device.readRegister(7200)
        else:
            raise StepperException("You cannot read the Quadrature Encoder when it is not connected.")

    def wiringDescription(self):
        """
        Name: Stepper.wiringDescription()

        Args: None

        Desc: Returns a string that describes how the stepper motor should be
              wired to the LabJack.

        Example:
        >>> from stepper import Stepper
        >>> d = stepper.Stepper(devType = 6)
        >>> print d.wiringDescription()

        Connection on DCA-10 -> Connection on LabJack
        IN1 -> FIO1
        IN2 -> FIO0
        EN  -> FIO2
        GND -> GND
        CS  -> AIN0
        """
        if self.encoderAttached:
            return UE9_WITH_ENCODER_WIRING_DESCRIPTION
        else:
            return UE9_WIRING_DESCRIPTION
