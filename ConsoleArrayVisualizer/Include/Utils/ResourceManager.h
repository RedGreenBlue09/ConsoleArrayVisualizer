#pragma once
#include "Utils/Tree234.h"

typedef uintptr_t handle_t;

typedef struct {
	tree234* ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle;
} resource_table_t;

void CreateResourceTable(resource_table_t* prtTable);
void DestroyResourceTable(resource_table_t* prtTable);

handle_t AddResource(resource_table_t* prtTable, void* pResource);
void* RemoveResource(resource_table_t* prtTable, handle_t Handle);

void* GetResource(resource_table_t* prtTable, handle_t Handle);
void* SetResource(resource_table_t* prtTable, handle_t Handle, void* pResourceData);

intptr_t GetResourceCount(resource_table_t* prtTable);
void** GetResourceList(resource_table_t* prtTable, intptr_t* pResourceCount);
