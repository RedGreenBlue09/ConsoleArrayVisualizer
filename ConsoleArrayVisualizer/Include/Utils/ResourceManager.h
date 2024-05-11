#pragma once
#include "Utils/Tree234.h"

typedef uintptr_t rm_handle_t;

#define INVALID_HANDLE (0) // reserved handle
#define FIRST_HANDLE (1)

typedef struct {
	tree234* ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle;
} resource_table_t;

void CreateResourceTable(resource_table_t* pResourceTable);
void DestroyResourceTable(resource_table_t* pResourceTable);

rm_handle_t AddResource(resource_table_t* pResourceTable, void* pResource);
void* RemoveResource(resource_table_t* pResourceTable, Visualizer_ArrayHandle hArray);

void* GetResource(resource_table_t* pResourceTable, Visualizer_ArrayHandle hArray);
void* SetResource(resource_table_t* pResourceTable, Visualizer_ArrayHandle hArray, void* pResourceData);

intptr_t GetResourceCount(resource_table_t* pResourceTable);
void** GetResourceList(resource_table_t* pResourceTable, intptr_t* pResourceCount);
