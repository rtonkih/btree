#pragma once
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
using namespace std;

#define M 3  // Order of B-tree: M link fields in each node

enum status {
	InsertNotComplete, Success, DuplicateKey,
	Underflow, NotFound
};

struct node {
	int n;        // Number of items stored in a node (n < M)
	int key[M - 1]; // Data items (only the first n in use)
	long child[M];    // 'Pointers' to other nodes (n+1 in use)
};

class BTree {
public:
	BTree(const char *TreeFileName);
	~BTree();
	void insert(int x);
	void insert(const char *InpFileName);
	void print() { cout << "Contents:\n"; pr(root, 0); }
	void DelNode(int x);
	void ShowSearch(int x);
private:
	enum { NIL = -1 };
	long root, FreeList;
	node RootNode;
	fstream file;
	status ins(long offset, int x, int &y, long &u);
	void pr(long offset, int nSpace);
	int NodeSearch(int x, const int *a, int n)const;
	status del(long offset, int x);
	void ReadNode(long offset, node &Node);
	void WriteNode(long offset, const node &Node);
	void ReadStart();
	long GetNode();
	void FreeNode(long r);
};
