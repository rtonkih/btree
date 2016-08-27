#include "BTree.h"

BTree::BTree(const char *TreeFileName)
{
	ifstream test(TreeFileName, ios::in | ios::_Nocreate);
	int NewFile = test.fail();
	test.clear(); test.close();
	if (NewFile)
	{
		file.open(TreeFileName, ios::out | ios::in |
			ios::trunc | ios::binary);
		root = FreeList = NIL;
		long start[2] = { NIL, NIL };
		file.write((char*)start, 2 * sizeof(long));
	}
	else
	{
		long start[2];
		file.open(TreeFileName, ios::out | ios::in |
			ios::_Nocreate | ios::binary);
		file.seekg(-1L, ios::end);
		char ch;
		file.read(&ch, 1); // Read signature.
		file.seekg(0L, ios::beg);
		file.read((char *)start, 2 * sizeof(long));
		if (ch != sizeof(int))
		{
			cout << "Wrong file format.\n"; exit(1);
		}
		root = start[0]; 
		FreeList = start[1];
		RootNode.n = 0;   // Signal for function ReadNode
		ReadNode(root, RootNode);
		//print();
	}
}

BTree::~BTree()
{
	long start[2];
	file.seekp(0L, ios::beg);
	start[0] = root; start[1] = FreeList;
	file.write((char*)start, 2 * sizeof(long));
	// The remaining code of this destructor is slightly
	// different from that in the first print of the book.
	// The length of the final binary file, including the 
	// signature byte at the end, will now always be an odd 
	// number, as it should be. There is a similar change in  
	// the function GetNode.I am grateful to Chian Wiz from 
	// Singapore, who showed me the possibility of a 'file leak', 
	// that is, an unused byte, which sometimes caused problems 
	// with the program 'showfile', when this was applied to 
	// this binary file. Such problems should no longer occur.
	// L. A.
	char ch = sizeof(int); // Signature
	file.seekg(0L, ios::end);
	if ((file.tellg() & 1) == 0)
		file.write(&ch, 1);
	// If the current file length is an even number, a
	// signature is added; otherwise it is already there.
	file.close();
}

void BTree::insert(int insertValue)
{
	long pNew;
	int xNew;
	status code = ins(root, insertValue, xNew, pNew);
	if (code == DuplicateKey)
		cout << "Duplicate key ignored.\n";
	if (code == InsertNotComplete)
	{
		long newRoot = root;
		root = GetNode(); //
		RootNode.n = 1;
		RootNode.key[0] = xNew;
		RootNode.child[0] = newRoot;
		RootNode.child[1] = pNew;
		WriteNode(root, RootNode);
	}
}

void BTree::insert(const char *InpFileName)
{
	ifstream InpFile(InpFileName, ios::in | ios::_Nocreate);
	if (InpFile.fail())
	{
		cout << "Cannot open input file " << InpFileName
			<< endl;
		return;
	}
	int insertValue;
	while (InpFile >> insertValue) {
		insert(insertValue);
	}
	InpFile.clear(); InpFile.close();
}

status BTree::ins(long offset, int insertValue, int &y, long &q)
{  // Insert x in *this. If not completely successful, the
   // integer y and the pointer q remain to be inserted.
   // Return value:
   //    Success, DuplicateKey or InsertNotComplete.
	long pNew, pFinal;
	int i, j, n;
	int xNew, kFinal;
	status code;
	if (offset == NIL) { q = NIL; y = insertValue; return InsertNotComplete; }
	node Node, NewNode;
	ReadNode(offset, Node);
	n = Node.n;
	i = NodeSearch(insertValue, Node.key, n);
	if (i < n && insertValue == Node.key[i]) {
		return DuplicateKey;
	}
	code = ins(Node.child[i], insertValue, xNew, pNew);
	if (code != InsertNotComplete) {
		return code;
	}
	// Insertion in suBTree did not completely succeed;
	// try to insert xNew and pNew in the current node:
	if (n < M - 1)
	{
		i = NodeSearch(xNew, Node.key, n);
		for (j = n; j > i; j--)
		{
			Node.key[j] = Node.key[j - 1];
			Node.child[j + 1] = Node.child[j];
		}
		Node.key[i] = xNew; 
		Node.child[i + 1] = pNew;
		++Node.n;
		WriteNode(offset, Node);
		return Success;
	}
	// Current node is full (n == M - 1) and will be split.
	// Pass item k[h] in the middle of the augmented
	// sequence back via parameter y, so that it
	// can move upward in the tree. Also, pass a pointer
	// to the newly created node back via parameter q:
	if (i == M - 1) { kFinal = xNew; pFinal = pNew; }
	else
	{
		kFinal = Node.key[M - 2]; 
		pFinal = Node.child[M - 1];
		for (j = M - 2; j > i; j--)
		{
			Node.key[j] = Node.key[j - 1]; 
			Node.child[j + 1] = Node.child[j];
		}
		Node.key[i] = xNew; 
		Node.child[i + 1] = pNew;
	}
	int h = (M - 1) / 2;
	y = Node.key[h];           // y and q are passed on to the
	q = GetNode();           // next higher level in the tree
							 // The values p[0],k[0],p[1],...,k[h-1],p[h] belong to
							 // the left of k[h] and are kept in *r:
	Node.n = h;
	// p[h+1],k[h+1],p[h+2],...,k[M-2],p[M-1],kFinal,pFinal
	// belong to the right of k[h] and are moved to *q:
	NewNode.n = M - 1 - h;
	for (j = 0; j < NewNode.n; j++)
	{
		NewNode.child[j] = Node.child[j + h + 1];
		NewNode.key[j] =
			(j < NewNode.n - 1 ? Node.key[j + h + 1] : kFinal);
	}
	NewNode.child[NewNode.n] = pFinal;
	WriteNode(offset, Node);
	WriteNode(q, NewNode);
	return InsertNotComplete;
}

void BTree::pr(long r, int nSpace)
{
	if (r != NIL)
	{
		int i;
		cout << setw(nSpace) << "";
		node Node;
		ReadNode(r, Node);
		for (i = 0; i < Node.n; i++) {
			cout << Node.key[i] << " ";
		}
		cout << endl;
		for (i = 0; i <= Node.n; i++) {
			pr(Node.child[i], nSpace + 8);
		}
	}
}

int BTree::NodeSearch(int insertValue, const int *a, int n)const
{
	int middle, left = 0, right = n - 1;
	if (insertValue <= a[left]) return 0;
	if (insertValue > a[right]) return n;
	while (right - left > 1)
	{
		middle = (right + left) / 2;
		(insertValue <= a[middle] ? right : left) = middle;
	}
	return right;
}

void BTree::ShowSearch(int x)
{
	int i, j, n;
	int counter = 0;
	long r = root;
	node Node;
	while (r != NIL)
	{
		ReadNode(r, Node);
		counter++;
		n = Node.n;
		//for (j = 0; j < Node.n; j++) {
		//cout << " " << Node.k[j];
		//}
		//cout << endl;
		i = NodeSearch(x, Node.key, n);
		if (i < n && x == Node.key[i])
		{
			cout << "Block read operation: " << counter << endl;
			cout << "Key " << x << " found in position " << i
				<< " of last displayed node.\n";
			return;
		}
		r = Node.child[i];
	}
	cout << "Key " << x << " not found.\n";
}

void BTree::DelNode(int x)
{
	long root0;
	switch (del(root, x))
	{
	case NotFound:
		cout << x << " not found.\n";
		break;
	case Underflow:
		root0 = root;
		root = RootNode.child[0]; FreeNode(root0);
		if (root != NIL) ReadNode(root, RootNode);
		break;
	}
}

status BTree::del(long r, int x)
{
	if (r == NIL) return NotFound;
	node Node;
	ReadNode(r, Node);
	int i, j, pivot, n = Node.n;
	int *k = Node.key;  // k[i] means Node.k[i]
	const int nMin = (M - 1) / 2;
	status code;
	long *p = Node.child, pL, pR;       // p[i] means Node.p[i]
	i = NodeSearch(x, k, n);
	if (p[0] == NIL)  // Are we dealing with a leaf?
	{
		if (i == n || x < k[i]) return NotFound;
		// x == k[i]
		for (j = i + 1; j < n; j++)
		{
			k[j - 1] = k[j]; p[j] = p[j + 1];
		}
		Node.n--;
		WriteNode(r, Node);
		return Node.n >= (r == root ? 1 : nMin) ?
			Success : Underflow;
	}
	// *r is an interior node, not a leaf:
	if (i < n && x == k[i])
	{  // x found in an interior node. Go to left child
	   // and follow a path all the way to a leaf,
	   // using rightmost branches:
		long q = p[i], q1; int nq; node Node1;
		for (;;)
		{
			ReadNode(q, Node1);
			nq = Node1.n; q1 = Node1.child[nq];
			if (q1 == NIL) break;
			q = q1;
		}
		// Exchange k[i] (= x) with rightmost item in leaf:
		k[i] = Node1.key[nq - 1];
		Node1.key[nq - 1] = x;
		WriteNode(r, Node); WriteNode(q, Node1);
	}
	// Delete x in leaf of suBTree with root p[i]:
	code = del(p[i], x);
	if (code != Underflow) return code;
	// There is underflow; borrow, and, if necessary, merge:
	// Too few data items in node *p[i]
	node NodeL, NodeR;
	if (i > 0)
	{
		pivot = i - 1; pL = p[pivot]; ReadNode(pL, NodeL);
		if (NodeL.n > nMin) // Borrow from left sibling
		{  // k[pivot] between pL and pR:
			pR = p[i];
			// Increase contents of *pR, borrowing from *pL:
			ReadNode(pR, NodeR);
			NodeR.child[NodeR.n + 1] = NodeR.child[NodeR.n];
			for (j = NodeR.n; j>0; j--)
			{
				NodeR.key[j] = NodeR.key[j - 1];
				NodeR.child[j] = NodeR.child[j - 1];
			}
			NodeR.n++;
			NodeR.key[0] = k[pivot];
			NodeR.child[0] = NodeL.child[NodeL.n];
			k[pivot] = NodeL.key[--NodeL.n];
			WriteNode(pL, NodeL); WriteNode(pR, NodeR);
			WriteNode(r, Node);
			return Success;
		}
	}
	pivot = i;
	if (i < n)
	{
		pR = p[pivot + 1]; ReadNode(pR, NodeR);
		if (NodeR.n > nMin) // Borrow from right sibling
		{  // k[pivot] between pL and pR:
			pL = p[pivot]; ReadNode(pL, NodeL);
			// Increase contents of *pL, borrowing from *pR:
			NodeL.key[NodeL.n] = k[pivot];
			NodeL.child[NodeL.n + 1] = NodeR.child[0];
			k[pivot] = NodeR.key[0];
			NodeL.n++; NodeR.n--;
			for (j = 0; j < NodeR.n; j++)
			{
				NodeR.key[j] = NodeR.key[j + 1];
				NodeR.child[j] = NodeR.child[j + 1];
			}
			NodeR.child[NodeR.n] = NodeR.child[NodeR.n + 1];
			WriteNode(pL, NodeL); WriteNode(pR, NodeR);
			WriteNode(r, Node);
			return Success;
		}
	}
	// Merge; neither borrow left nor borrow right possible.
	pivot = (i == n ? i - 1 : i);
	pL = p[pivot]; pR = p[pivot + 1];
	// Add k[pivot] and *pR to *pL:
	ReadNode(pL, NodeL); ReadNode(pR, NodeR);
	NodeL.key[NodeL.n] = k[pivot];
	NodeL.child[NodeL.n + 1] = NodeR.child[0];
	for (j = 0; j < NodeR.n; j++)
	{
		NodeL.key[NodeL.n + 1 + j] = NodeR.key[j];
		NodeL.child[NodeL.n + 2 + j] = NodeR.child[j + 1];
	}
	NodeL.n += 1 + NodeR.n;
	FreeNode(pR);
	for (j = i + 1; j < n; j++)
	{
		k[j - 1] = k[j]; p[j] = p[j + 1];
	}
	Node.n--;
	WriteNode(pL, NodeL); WriteNode(r, Node);
	return
		Node.n >= (r == root ? 1 : nMin) ? Success : Underflow;
}

void BTree::ReadNode(long offset, node &Node)
{
	if (offset == NIL) return;
	if (offset == root && RootNode.n > 0) {
		Node = RootNode;
	}
	else {
		file.seekg(offset, ios::beg);
		file.read((char*)&Node, sizeof(node));
	}
}

void BTree::WriteNode(long offset, const node &Node)
{
	if (offset == root) RootNode = Node;
	file.seekp(offset, ios::beg);
	file.write((char*)&Node, sizeof(node));
}

void BTree::ReadStart()
{
	long start[2];
	file.seekg(0L, ios::beg);
	file.read((char *)start, 2 * sizeof(long));
	root = start[0]; FreeList = start[1];
	ReadNode(root, RootNode);
}

long BTree::GetNode()  // Modified (see also the destructor ~BTree)
{
	long offset;
	node Node;
	if (FreeList == NIL)
	{
		file.seekp(0L, ios::end); // Allocate space on disk; if
		offset = file.tellp() & ~1;    // file length is an odd number,
		WriteNode(offset, Node);       // the new node will overwrite
	}
	else                      // signature byte at end of file
	{
		offset = FreeList;
		ReadNode(offset, Node);        // To update FreeList:
		FreeList = Node.child[0];     // Reduce the free list by 1
	}
	return offset;
}

void BTree::FreeNode(long r)
{
	node Node;
	ReadNode(r, Node);
	Node.child[0] = FreeList;
	FreeList = r;
	WriteNode(r, Node);
}
