#include "./import.h"

ImportNode* CreateOrGetImportNode(Interpreter* interpreter, String module) {
	ImportNode* node = interpreter->ImportHead;
	while (node != NULL) {
		if (strcmp(node->Path, module) == 0) {
			return node;
		}
		node = node->Next;
	}

	// Node not found, create it
	node			   = Allocate(sizeof(ImportNode));
	node->Path		   = AllocateString(module);
	node->Dependencies = NULL;
	node->DepCount	   = 0;
	node->State		   = IMPORT_UNVISITED;

	// Prepend to the linked list
	node->Next				= interpreter->ImportHead;
	interpreter->ImportHead = node;

	return node;
}

void ImportNodeAddDependency(ImportNode* dst, ImportNode* src) {
	if (dst->DepCount == 0) {
		dst->Dependencies = Allocate(sizeof(ImportNode*));
	}

	dst->Dependencies[dst->DepCount++] = (ImportNode*) src;
	dst->Dependencies = Reallocate(dst->Dependencies,
								   sizeof(ImportNode*) * (dst->DepCount + 1));
}

bool ImportNodeHasCircularDependency(ImportNode* node, String modulePath) {
	if (strcmp(node->Path, modulePath) == 0) {
		return true;
	}

	for (int i = 0; i < node->DepCount; i++) {
		ImportNode* dep = node->Dependencies[i];

		// Skip nodes we already proved are completely safe
		if (dep->State == SAFE)
			continue;

		// If we hit a node currently in our traversal stack,
		// it's a cycle
		if (dep->State == VISITING)
			return true;

		// Mark as currently visiting
		dep->State = VISITING;

		if (ImportNodeHasCircularDependency(dep, modulePath)) {
			return true;
		}

		// CRITICAL: Mark as Safe. Do not reset to 0.
		// We know for a fact this branch does not lead to
		// modulePath.
		dep->State = SAFE;
	}

	return false;
}

void FreeImportNode(ImportNode* node) {
	if (node == NULL)
		return;

	while (node != NULL) {
		if (node->Dependencies != NULL) {
			free(node->Dependencies);
		}
		free(node->Path);
		ImportNode* next = node->Next;
		free(node);
		node = next;
	}
}