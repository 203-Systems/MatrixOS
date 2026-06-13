import MatrixOS


print("MatrixOS Python API Introspection")

print("LED count", MatrixOS.LED.count())
for partition in MatrixOS.LED.partitions():
    print("LED partition", partition.index, partition.name, partition.start, partition.size, partition.type)

grid = MatrixOS.Input.primary_grid()
if grid is not None:
    grid_id = grid.cluster_id()
    grid_name = grid.name()
    grid_width = grid.width()
    grid_height = grid.height()
    grid_inputs = grid.input_count()
    print("Primary grid", grid_id, grid_name, grid_width, grid_height, grid_inputs)

for cluster in MatrixOS.Input.keypad_clusters():
    cluster_id = cluster.cluster_id()
    cluster_name = cluster.name()
    cluster_inputs = cluster.input_count()
    print("Keypad cluster", cluster_id, cluster_name, cluster_inputs)

packet = MatrixOS.MIDI.note_on(0, 60, 100)
velocity = MatrixOS.MIDI.velocity(packet)
print("MIDI velocity", velocity)
