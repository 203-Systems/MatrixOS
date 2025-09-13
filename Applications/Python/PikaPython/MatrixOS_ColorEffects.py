# ColorEffects wrapper for MatrixOS
# Binding of OS/Framework/Color/ColorEffects.h

import _MatrixOS_ColorEffects
from MatrixOS_Color import Color

# Color generation effects
def Rainbow(period: int = 1000, offset: int = 0) -> Color:
    """Generate a rainbow color that cycles through the spectrum"""
    return _MatrixOS_ColorEffects.Rainbow(period, offset)

# Brightness modulation effects (returns 0-255)
def Breath(period: int = 1000, offset: int = 0) -> int:
    """Generate a breathing brightness value (0-255)"""
    return _MatrixOS_ColorEffects.Breath(period, offset)

def BreathLowBound(low_bound: int = 64, period: int = 1000, offset: int = 0) -> int:
    """Generate a breathing brightness value with a lower bound"""
    return _MatrixOS_ColorEffects.BreathLowBound(low_bound, period, offset)

def Strobe(period: int = 1000, offset: int = 0) -> int:
    """Generate a strobe brightness value (0 or 255)"""
    return _MatrixOS_ColorEffects.Strobe(period, offset)

def Saw(period: int = 1000, offset: int = 0) -> int:
    """Generate a sawtooth wave brightness value (0-255)"""
    return _MatrixOS_ColorEffects.Saw(period, offset)

def Triangle(period: int = 1000, offset: int = 0) -> int:
    """Generate a triangle wave brightness value (0-255)"""
    return _MatrixOS_ColorEffects.Triangle(period, offset)

# Color modulation effects (applies effect to existing color)
def ColorBreath(color: Color, period: int = 1000, offset: int = 0) -> Color:
    """Apply breathing effect to a color"""
    return _MatrixOS_ColorEffects.ColorBreath(color, period, offset)

def ColorBreathLowBound(color: Color, low_bound: int = 64, period: int = 1000, offset: int = 0) -> Color:
    """Apply breathing effect with lower bound to a color"""
    return _MatrixOS_ColorEffects.ColorBreathLowBound(color, low_bound, period, offset)

def ColorStrobe(color: Color, period: int = 1000, offset: int = 0) -> Color:
    """Apply strobe effect to a color"""
    return _MatrixOS_ColorEffects.ColorStrobe(color, period, offset)

def ColorSaw(color: Color, period: int = 1000, offset: int = 0) -> Color:
    """Apply sawtooth wave effect to a color"""
    return _MatrixOS_ColorEffects.ColorSaw(color, period, offset)

def ColorTriangle(color: Color, period: int = 1000, offset: int = 0) -> Color:
    """Apply triangle wave effect to a color"""
    return _MatrixOS_ColorEffects.ColorTriangle(color, period, offset)