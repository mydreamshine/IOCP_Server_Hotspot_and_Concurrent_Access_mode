#pragma once
#include "..\..\2019_TT\2019_TT\protocol.h"
#include <atomic>

void InitializeNetwork();
void ShutdownNetwork();
void GetPointCloud(int *size, float **points);