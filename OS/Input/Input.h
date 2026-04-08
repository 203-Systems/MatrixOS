#pragma once

namespace MatrixOS::Input
{
void Init();
bool NewEvent(const InputEvent& event);
void RegisterCluster(const InputCluster& cluster);
void ClearClusters();
} // namespace MatrixOS::Input
