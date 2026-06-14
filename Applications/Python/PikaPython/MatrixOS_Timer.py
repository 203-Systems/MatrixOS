class Timer:
    def __init__(self):
        import MatrixOS_SYS as SYS
        self.last_tick = SYS.millis()

    def tick(self, ms: int, continuous_mode: bool = False) -> bool:
        import MatrixOS_SYS as SYS
        now = SYS.millis()
        if now - self.last_tick < ms:
            return False

        if continuous_mode:
            self.last_tick += ms
        else:
            self.last_tick = now
        return True

    def since_last_tick(self) -> int:
        import MatrixOS_SYS as SYS
        return SYS.millis() - self.last_tick

    def record_current(self) -> None:
        import MatrixOS_SYS as SYS
        self.last_tick = SYS.millis()
