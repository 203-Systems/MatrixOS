#!/usr/bin/env python3
"""
Test script for UIComponent Python bindings in MatrixOS
Tests the SetEnabled, SetEnableFunc callback, and TryCallback functions
"""

from MatrixOS_UIComponent import UIComponent

def test_basic_component():
    """Test basic UIComponent creation and enabled state"""
    print("Testing basic UIComponent...")

    # Create component
    component = UIComponent()

    # Test SetEnabled
    component.SetEnabled(True)
    print("  Set enabled to True")

    component.SetEnabled(False)
    print("  Set enabled to False")

    # Test TryCallback without a callback set
    result = component.TryCallback()
    print(f"  TryCallback result (no callback): {result}")
    assert result == False, "Should be False when disabled and no callback"

    component.SetEnabled(True)
    result = component.TryCallback()
    print(f"  TryCallback result (enabled, no callback): {result}")
    assert result == True, "Should be True when enabled and no callback"

    print("  ✓ Basic component test passed")
    return True


def test_callback_functionality():
    """Test the SetEnableFunc callback mechanism"""
    print("\nTesting callback functionality...")

    component = UIComponent()

    # Create a simple state for testing
    test_state = {"counter": 0}

    # Define a callback that checks if counter is even
    def is_counter_even():
        return test_state["counter"] % 2 == 0

    # Set the callback
    component.SetEnableFunc(is_counter_even)
    print("  Set callback to check if counter is even")

    # Test with counter = 0 (even)
    test_state["counter"] = 0
    result = component.TryCallback()
    print(f"  Counter=0: TryCallback result: {result}")
    assert result == True, "Should be True when counter is 0 (even)"

    # Test with counter = 1 (odd)
    test_state["counter"] = 1
    result = component.TryCallback()
    print(f"  Counter=1: TryCallback result: {result}")
    assert result == False, "Should be False when counter is 1 (odd)"

    # Test with counter = 2 (even)
    test_state["counter"] = 2
    result = component.TryCallback()
    print(f"  Counter=2: TryCallback result: {result}")
    assert result == True, "Should be True when counter is 2 (even)"

    print("  ✓ Callback functionality test passed")
    return True


def test_multiple_components():
    """Test multiple components with different callbacks"""
    print("\nTesting multiple components...")

    # Create multiple components with different enable logic
    components = []

    for i in range(3):
        comp = UIComponent()

        # Each component has its own enable logic
        # Using a closure to capture the index
        def make_callback(index):
            def callback():
                # Component is enabled if its index matches a global selector
                return globals().get('selected_component', 0) == index
            return callback

        comp.SetEnableFunc(make_callback(i))
        components.append(comp)
        print(f"  Created component {i} with selection-based callback")

    # Test enabling different components
    for selected in range(3):
        globals()['selected_component'] = selected
        print(f"  Selected component: {selected}")

        for i, comp in enumerate(components):
            result = comp.TryCallback()
            expected = (i == selected)
            print(f"    Component {i}: {result} (expected: {expected})")
            assert result == expected, f"Component {i} should be {'enabled' if expected else 'disabled'}"

    print("  ✓ Multiple components test passed")
    return True


def test_dynamic_callback_update():
    """Test updating callbacks dynamically"""
    print("\nTesting dynamic callback updates...")

    component = UIComponent()

    # First callback - always returns True
    def always_true():
        return True

    component.SetEnableFunc(always_true)
    result = component.TryCallback()
    print(f"  With always_true callback: {result}")
    assert result == True, "Should be True with always_true callback"

    # Update to different callback - always returns False
    def always_false():
        return False

    component.SetEnableFunc(always_false)
    result = component.TryCallback()
    print(f"  With always_false callback: {result}")
    assert result == False, "Should be False with always_false callback"

    # Update to conditional callback
    toggle_state = {"enabled": True}

    def toggle_callback():
        return toggle_state["enabled"]

    component.SetEnableFunc(toggle_callback)

    toggle_state["enabled"] = True
    result = component.TryCallback()
    print(f"  With toggle callback (True): {result}")
    assert result == True, "Should be True when toggle is True"

    toggle_state["enabled"] = False
    result = component.TryCallback()
    print(f"  With toggle callback (False): {result}")
    assert result == False, "Should be False when toggle is False"

    print("  ✓ Dynamic callback update test passed")
    return True


def main():
    """Run all tests"""
    print("=" * 50)
    print("UIComponent Python Binding Tests")
    print("=" * 50)

    try:
        # Run all test functions
        tests = [
            test_basic_component,
            test_callback_functionality,
            test_multiple_components,
            test_dynamic_callback_update
        ]

        passed = 0
        failed = 0

        for test in tests:
            try:
                if test():
                    passed += 1
            except Exception as e:
                print(f"  ✗ Test failed with error: {e}")
                failed += 1

        print("\n" + "=" * 50)
        print(f"Test Results: {passed} passed, {failed} failed")
        print("=" * 50)

        if failed == 0:
            print("\n✓ All tests passed successfully!")
            return 0
        else:
            print(f"\n✗ {failed} test(s) failed")
            return 1

    except ImportError as e:
        print(f"Import error: {e}")
        print("Make sure MatrixOS Python bindings are properly built and installed")
        return 2


if __name__ == "__main__":
    exit(main())