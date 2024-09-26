#pragma once

/*
 * This code was originally written by David Klaftenegger in 2015.
 * It was modified by Edvin Bruce and Viktor Wallsten in 2024 as part of an assignment.
 */

#include <mutex>
#include <atomic>
#include <functional>
#include <pthread.h>

class QNode
{
public:
    std::atomic<bool> locked;
    QNode() : locked(false) {}
};

class CLHLock2
{
private:
    std::atomic<QNode*> tail;
    std::mutex out;

public:
    CLHLock2()
    {
        tail.store(new QNode());
    }
    ~CLHLock2()
    {
        delete tail.load();
    }

    void lock(QNode*& myNode, QNode*& myPred)
    {
        myNode->locked.store(true);
        // out.lock();
        // std::cout << pthread_self() << " thread is trying to lock" << std::endl;
        // std::cout << "tail: " << tail.load() << " - " << tail.load()->locked.load() << std::endl;
        // std::cout << "myNode: " << myNode << " - " << myNode->locked.load() << std::endl;
        // out.unlock();

        myPred = tail.exchange(myNode);

        // out.lock();
        // std::cout << "myPred: " << myPred << " - " << myPred->locked.load() << std::endl;
        // out.unlock();

        // out.lock();
        // std::cout << pthread_self() << " starting busy-wait" << std::endl;
        // out.unlock();

        int local_wait_counter = 0;
        while (myPred->locked.load())
        {
            local_wait_counter++;
            // Busy-wait
        }

        // out.lock();
        // std::cout << pthread_self() << " exited busy-wait after " << local_wait_counter << " cycles." << std::endl;
        // out.unlock();
    }

    void unlock(QNode*& myNode, QNode*& myPred)
    {
        // out.lock();
        // std::cout << pthread_self() << " unlocking CLH lock" << std::endl;
        // std::cout << "myNode before setting locked to false: " << myNode << " - " << myNode->locked.load() << std::endl;
        myNode->locked.store(false);
        // std::cout << "myNode after setting locked to false: " << myNode << " - " << myNode->locked.load() << std::endl;
        // std::cout << "tail after unlocking: " << tail.load() << " - " << tail.load()->locked.load() << std::endl;
        // out.unlock();

        // Reuse myPred as myNode for the next lock acquisition
        myNode = myPred;
    }
};


struct Node
{
	std::atomic<bool> locked{false};
};

// thread_local static Node* myNode = nullptr;
// thread_local static Node* myPrevNode = nullptr;

class CLHLock
{
private:
	std::atomic<Node *> tail;
	Node *myNode;
	Node *myPrevNode;

public:
	CLHLock()
	{
		Node *initialNode = new Node();
		initialNode->locked.store(false);
		tail.store(initialNode);
	}

	void lock()
	{
		myNode = new Node();
		myNode->locked.store(true);
		Node *prev = tail.exchange(myNode);
		myPrevNode = prev;
		while (prev->locked.load())
		{
			// Busy-wait
		}
	}

	void unlock()
	{
		myNode->locked.store(false);
		// myNode = myPrevNode;
	}

	~CLHLock()
	{
		delete tail.load();
	}
};

class TATASLock
{
private:
	std::atomic<bool> locked{false}; // Initially unlocked

public:
	void lock()
	{
		while (true)
		{
			// Wait until lock looks free
			while (locked.load())
			{
				// Spin here until it might become unlocked
			}
			// try to take the lock
			if (!locked.exchange(true))
			{
				break; // Lock acquired successfully
			}
		}
	}

	void unlock()
	{
		locked.store(false); // Unlock the lock
	}
};

template <typename T>
struct node
{
	T value;
	node<T> *next;
};

template <typename T>
struct node_mutex // used in fine grained mutex list
{
	T value;
	node_mutex<T> *next;
	std::mutex mutex;
};

template <typename T>
struct node_tatas // used in fine grained mutex list
{
	T value;
	node_tatas<T> *next;
	TATASLock tatas_lock;
};

template <typename T>
struct node_clh // used in fine grained mutex list
{
	T value;
	node_clh<T> *next;
	CLHLock2 clh_lock;
};

template <typename T>
class list_superclass
{
public:
	virtual void insert(T v) = 0;
	virtual void remove(T v) = 0;
	virtual std::size_t count(T v) = 0;
	virtual ~list_superclass() {

	};
};

template <typename T>
class cg_mutex_sorted_list : public list_superclass<T>
{ // cg = coarse grained
public:
	cg_mutex_sorted_list() = default;
	cg_mutex_sorted_list(const cg_mutex_sorted_list<T> &other) = default;
	cg_mutex_sorted_list(cg_mutex_sorted_list<T> &&other) = default;
	cg_mutex_sorted_list<T> &operator=(const cg_mutex_sorted_list<T> &other) = default;
	cg_mutex_sorted_list<T> &operator=(cg_mutex_sorted_list<T> &&other) = default;
	~cg_mutex_sorted_list()
	{
		while (first != nullptr)
		{
			remove(first->value);
		}
	}

	/* insert v into the list */
	void insert(T v)
	{
		mutex_lock.lock();
		/* first find position */
		node<T> *pred = nullptr;
		node<T> *succ = first;
		while (succ != nullptr && succ->value < v)
		{
			pred = succ;
			succ = succ->next;
		}

		/* construct new node */
		node<T> *current = new node<T>();
		current->value = v;

		/* insert new node between pred and succ */
		current->next = succ;
		if (pred == nullptr)
		{
			first = current;
		}
		else
		{
			pred->next = current;
		}
		mutex_lock.unlock();
	}

	void remove(T v)
	{
		mutex_lock.lock();

		/* first find position */
		node<T> *pred = nullptr;
		node<T> *current = first;
		while (current != nullptr && current->value < v)
		{
			pred = current;
			current = current->next;
		}
		if (current == nullptr || current->value != v)
		{
			/* v not found */
			mutex_lock.unlock();
			return;
		}
		/* remove current */
		if (pred == nullptr)
		{
			first = current->next;
		}
		else
		{
			pred->next = current->next;
		}
		delete current;
		mutex_lock.unlock();
	}

	/* count elements with value v in the list */
	std::size_t count(T v)
	{
		mutex_lock.lock();
		std::size_t cnt = 0;
		/* first go to value v */
		node<T> *current = first;
		while (current != nullptr && current->value < v)
		{
			current = current->next;
		}
		/* count elements */
		while (current != nullptr && current->value == v)
		{
			cnt++;
			current = current->next;
		}
		mutex_lock.unlock();
		return cnt;
	}

private:
	std::mutex mutex_lock; // this locks the entire list. I consider this to be very coarse.
	node<T> *first = nullptr;
};

// TODO: Auctually lock the locks in the nodes. It should be as easy as to just add the lock whenever you acces a mutex kinda
template <typename T>
class fg_mutex_sorted_list : public list_superclass<T>
{ // fg = fine grained
public:
	fg_mutex_sorted_list() = default;
	fg_mutex_sorted_list(const fg_mutex_sorted_list<T> &other) = default;
	fg_mutex_sorted_list(fg_mutex_sorted_list<T> &&other) = default;
	fg_mutex_sorted_list<T> &operator=(const fg_mutex_sorted_list<T> &other) = default;
	fg_mutex_sorted_list<T> &operator=(fg_mutex_sorted_list<T> &&other) = default;
	~fg_mutex_sorted_list()
	{
		node_mutex<T> *current = first;
		while (current != nullptr)
		{
			node_mutex<T> *next = current->next;
			delete current;
			current = next;
		}
	}
	void insert(T v)
	{
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting insert, trying to insert value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in insert" << std::endl;
		// out.unlock();
		list_lock.lock();
		// out.lock();
		// std::cout << pthread_self() << " list locked in insert" << std::endl;
		// out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;

		if (first != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first in insert, value: " << first->value << std::endl;
			// out.unlock();
			first->mutex.lock();
			// out.lock();
			// std::cout << pthread_self() << " first locked in insert, value: " << first->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl << std::endl;
			// out.unlock();
		}

		node_mutex<T> *pred = nullptr;
		node_mutex<T> *succ = first;

		// out.lock();
		// std::cout << pthread_self() << " before while loop in insert" << std::endl;
		// out.unlock();

		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->mutex.unlock();
			}

			if (list_lock_locked)
			{
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}

			pred = succ;
			succ = succ->next;

			if (succ != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking succ: " << succ->value << std::endl;
				// out.unlock();
				succ->mutex.lock();
			}
		}
		// previous or list_lock is now locked (if we are at the beginning of the list)
		// succ may be locked or is nullptr (if we are at end of list)
		// out.lock();
		// std::cout << pthread_self() << " succ: " << succ << std::endl;
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		node_mutex<T> *new_node = new node_mutex<T>();
		new_node->value = v;

		if (list_lock_locked)
		{
			// if the list_lock is still locked, we should insert the new node at the beginning of the list
			new_node->next = first;
			first = new_node;
			if (new_node->next != nullptr)
			{
				new_node->next->mutex.unlock();
			}
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			// out.unlock();
			list_lock.unlock();
		}
		else if (pred != nullptr && succ == nullptr)
		{
			// if we are at the end of the list. Now only pred is locked
			pred->next = new_node;
			new_node->next = nullptr;
			pred->mutex.unlock();
		}
		else if (pred != nullptr && succ != nullptr)
		{
			// if we are in the middle of the list. Now pred and succ is locked
			pred->next = new_node;
			new_node->next = succ;
			succ->mutex.unlock();
			pred->mutex.unlock();
		}
		else
		{
			out.lock();
			std::cout << pthread_self() << " Error in insert" << std::endl;
			out.unlock();
			exit(1);
		}
	}

	void remove(T v)
	{
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting remove, trying to remove value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in remove" << std::endl;
		// out.unlock();
		list_lock.lock();
		// out.lock();
		// std::cout << pthread_self() << " list locked in remove" << std::endl;
		// out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;

		if (first != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first in remove, value: " << first->value << std::endl;
			// out.unlock();
			first->mutex.lock();
			// out.lock();
			// std::cout << pthread_self() << " first locked in remove value: " << first->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl;
			// out.unlock();
		}

		node_mutex<T> *pred = nullptr;
		node_mutex<T> *current = first;

		// out.lock();
		// std::cout << std::endl << pthread_self() << " before while loop in remove" << std::endl;
		// out.unlock();

		while (current != nullptr && current->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->mutex.unlock();
			}

			if (list_lock_locked)
			{
				list_lock_locked = false;
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
			}

			pred = current;
			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				current->mutex.lock();
			}
		}

		// previous or list_lock is now locked (if we are at the beginning of the list)
		// current may be locked or is nullptr (if we are at end of list)

		// if we never entered the while loop (list is empty or the first element is larger than the one we are trying to remove).
		// if the list_lock is still locked, we are at the beginning of the list

		// out.lock();
		// std::cout << std::endl << pthread_self() << " after while loop in remove" << std::endl;
		//
		// if (current != nullptr)
		// {
		// 	std::cout << pthread_self() << " current->value: " << current->value << std::endl;
		// }
		// else {
		// 	std::cout << pthread_self() << " current: " << current << std::endl;
		// }
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		if (list_lock_locked)
		{
			if (first != nullptr)
			{
				// there is a first element which we might want to remove
				if (first->value == v)
				{
					// we want to remove the first element
					node_mutex<T> *node_to_remove = first;
					first = first->next;
					delete node_to_remove; // dont need to unlock the mutex since we are deleting the node. No other thread should have access to it.
				}
				else
				{
					// if first value exists but is not the value we want to remove
					first->mutex.unlock();
				}
			}
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			// out.unlock();
			list_lock.unlock();
		}
		// if we reach the end of the list.
		else if (current == nullptr)
		{
			// current is not locked, but previous may be locked
			if (pred != nullptr)
			{
				pred->mutex.unlock();
			}
		}
		else
		{
			// we are in the middle of the list, and we might have found the element we want to remove
			// current and previous is locked. list_lock is not locked
			// out.lock();
			// std::cout << pthread_self() << " current->value: " << current->value << std::endl;
			// std::cout << pthread_self() << " v: " << v << std::endl;
			// std::cout << pthread_self() << " pred->value: " << pred->value << std::endl;
			// out.unlock();
			if (current->value == v)
			{
				// out.lock();
				// std::cout << pthread_self() << " deleting current with value: " << current->value << std::endl;
				// out.unlock();
				// we want to remove current
				pred->next = current->next;
				delete current;
			}
			else
			{
				current->mutex.unlock();
			}
			pred->mutex.unlock();
		}
	}

	std::size_t count(T v)
	{
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting count, trying to count value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in count" << std::endl;
		// out.unlock();
		list_lock.lock();
		// out.lock();
		// std::cout << pthread_self() << " list locked in count" << std::endl;
		// out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;

		if (first != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first in count, value: " << first->value << std::endl;
			// out.unlock();
			first->mutex.lock();
			// out.lock();
			// std::cout << pthread_self() << " first locked in count, value: " << first->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl << std::endl;
			// out.unlock();
		}

		node_mutex<T> *pred = nullptr;
		node_mutex<T> *current = first;

		// out.lock();
		// std::cout << pthread_self() << " before while loop in count" << std::endl;
		// out.unlock();

		while (current != nullptr && current->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->mutex.unlock();
			}

			if (list_lock_locked)
			{
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}

			pred = current;

			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				current->mutex.lock();
			}
		}

		// out.lock();
		// std::cout << std::endl << pthread_self() << " after while loop in count" << std::endl;
		//
		// if (current != nullptr)
		// {
		// 	std::cout << pthread_self() << " current->value: " << current->value << std::endl;
		// }
		// else {
		// 	std::cout << pthread_self() << " current: " << current << std::endl;
		// }
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		if (current == nullptr)
		{
			if (list_lock_locked)
			{
				// we never entered the while loop, list is empty or the first element is greater than or equal to the value we are looking for
				list_lock.unlock();
			}
			else if (current == nullptr)
			{
				// we have reached the end of the list
				if (pred != nullptr)
				{
					pred->mutex.unlock();
				}
			}
			return 0;
		}

		std::size_t counter = 0;

		// now we start counting
		// current and pred is locked
		while (current != nullptr && current->value == v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->mutex.unlock();
			}
			if (list_lock_locked) // if we wanna count instances of the first element, list_lock will be locked.
			{
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}
			counter++;
			pred = current;
			current = current->next;
			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				current->mutex.lock();
			}
		}

		// unlock locks if any are still locked, as we are done counting. this is kinda dogwater but works
		if (current != nullptr)
		{
			current->mutex.unlock();
		}
		if (pred != nullptr)
		{
			pred->mutex.unlock();
		}
		if (list_lock_locked)
		{
			list_lock.unlock();
		}
		return counter;
	}

private:
	node_mutex<T> *first = nullptr;
	std::mutex out;
	std::mutex list_lock; // this locks new threads from accesing the list
};

template <typename T>
class cg_tatas_sorted_list : public list_superclass<T>
{ // cg = coarse grained
public:
	cg_tatas_sorted_list() = default;
	cg_tatas_sorted_list(const cg_tatas_sorted_list<T> &other) = default;
	cg_tatas_sorted_list(cg_tatas_sorted_list<T> &&other) = default;
	cg_tatas_sorted_list<T> &operator=(const cg_tatas_sorted_list<T> &other) = default;
	cg_tatas_sorted_list<T> &operator=(cg_tatas_sorted_list<T> &&other) = default;
	~cg_tatas_sorted_list()
	{
		while (first != nullptr)
		{
			remove(first->value);
		}
	}

	/* insert v into the list */
	void insert(T v)
	{
		tatas_lock.lock();
		/* first find position */
		node<T> *pred = nullptr;
		node<T> *succ = first;
		while (succ != nullptr && succ->value < v)
		{
			pred = succ;
			succ = succ->next;
		}

		/* construct new node */
		node<T> *current = new node<T>();
		current->value = v;

		/* insert new node between pred and succ */
		current->next = succ;
		if (pred == nullptr)
		{
			first = current;
		}
		else
		{
			pred->next = current;
		}
		tatas_lock.unlock();
	}

	void remove(T v)
	{
		tatas_lock.lock();

		/* first find position */
		node<T> *pred = nullptr;
		node<T> *current = first;
		while (current != nullptr && current->value < v)
		{
			pred = current;
			current = current->next;
		}
		if (current == nullptr || current->value != v)
		{
			/* v not found */
			tatas_lock.unlock();
			return;
		}
		/* remove current */
		if (pred == nullptr)
		{
			first = current->next;
		}
		else
		{
			pred->next = current->next;
		}
		delete current;
		tatas_lock.unlock();
	}

	/* count elements with value v in the list */
	std::size_t count(T v)
	{
		tatas_lock.lock();
		std::size_t cnt = 0;
		/* first go to value v */
		node<T> *current = first;
		while (current != nullptr && current->value < v)
		{
			current = current->next;
		}
		/* count elements */
		while (current != nullptr && current->value == v)
		{
			cnt++;
			current = current->next;
		}
		tatas_lock.unlock();
		return cnt;
	}

private:
	TATASLock tatas_lock;
	node<T> *first = nullptr;
};

template <typename T>
class fg_tatas_sorted_list : public list_superclass<T>
{ // fg = fine grained
public:
	fg_tatas_sorted_list() = default;
	fg_tatas_sorted_list(const fg_tatas_sorted_list<T> &other) = default;
	fg_tatas_sorted_list(fg_tatas_sorted_list<T> &&other) = default;
	fg_tatas_sorted_list<T> &operator=(const fg_tatas_sorted_list<T> &other) = default;
	fg_tatas_sorted_list<T> &operator=(fg_tatas_sorted_list<T> &&other) = default;
	~fg_tatas_sorted_list()
	{
		while (first != nullptr)
		{
			remove(first->value);
		}
	}
	void insert(T v)
	{
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting insert, trying to insert value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in insert" << std::endl;
		// out.unlock();
		list_lock.lock();
		// out.lock();
		// std::cout << pthread_self() << " list locked in insert" << std::endl;
		// out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;

		if (first != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first in insert, value: " << first->value << std::endl;
			// out.unlock();
			first->tatas_lock.lock();
			// out.lock();
			// std::cout << pthread_self() << " first locked in insert, value: " << first->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl << std::endl;
			// out.unlock();
		}

		node_tatas<T> *pred = nullptr;
		node_tatas<T> *succ = first;

		// out.lock();
		// std::cout << pthread_self() << " before while loop in insert" << std::endl;
		// out.unlock();

		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->tatas_lock.unlock();
			}

			if (list_lock_locked)
			{
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}

			pred = succ;
			succ = succ->next;

			if (succ != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking succ: " << succ->value << std::endl;
				// out.unlock();
				succ->tatas_lock.lock();
			}
		}
		// previous or list_lock is now locked (if we are at the beginning of the list)
		// succ may be locked or is nullptr (if we are at end of list)
		// out.lock();
		// std::cout << pthread_self() << " succ: " << succ << std::endl;
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		node_tatas<T> *new_node = new node_tatas<T>();
		new_node->value = v;

		if (list_lock_locked)
		{
			// if the list_lock is still locked, we should insert the new node at the beginning of the list
			new_node->next = first;
			first = new_node;
			if (new_node->next != nullptr)
			{
				new_node->next->tatas_lock.unlock();
			}
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			// out.unlock();
			list_lock.unlock();
		}
		else if (pred != nullptr && succ == nullptr)
		{
			// if we are at the end of the list. Now only pred is locked
			pred->next = new_node;
			new_node->next = nullptr;
			pred->tatas_lock.unlock();
		}
		else if (pred != nullptr && succ != nullptr)
		{
			// if we are in the middle of the list. Now pred and succ is locked
			pred->next = new_node;
			new_node->next = succ;
			succ->tatas_lock.unlock();
			pred->tatas_lock.unlock();
		}
		else
		{
			out.lock();
			std::cout << pthread_self() << " Error in insert" << std::endl;
			out.unlock();
			exit(1);
		}
	}

	void remove(T v)
	{
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting remove, trying to remove value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in remove" << std::endl;
		// out.unlock();
		list_lock.lock();
		// out.lock();
		// std::cout << pthread_self() << " list locked in remove" << std::endl;
		// out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;

		if (first != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first in remove, value: " << first->value << std::endl;
			// out.unlock();
			first->tatas_lock.lock();
			// out.lock();
			// std::cout << pthread_self() << " first locked in remove value: " << first->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl;
			// out.unlock();
		}

		node_tatas<T> *pred = nullptr;
		node_tatas<T> *current = first;

		// out.lock();
		// std::cout << std::endl << pthread_self() << " before while loop in remove" << std::endl;
		// out.unlock();

		while (current != nullptr && current->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->tatas_lock.unlock();
			}

			if (list_lock_locked)
			{
				list_lock_locked = false;
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
			}

			pred = current;
			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				current->tatas_lock.lock();
			}
		}

		// previous or list_lock is now locked (if we are at the beginning of the list)
		// current may be locked or is nullptr (if we are at end of list)

		// if we never entered the while loop (list is empty or the first element is larger than the one we are trying to remove).
		// if the list_lock is still locked, we are at the beginning of the list

		// out.lock();
		// std::cout << std::endl << pthread_self() << " after while loop in remove" << std::endl;
		//
		// if (current != nullptr)
		// {
		// 	std::cout << pthread_self() << " current->value: " << current->value << std::endl;
		// }
		// else {
		// 	std::cout << pthread_self() << " current: " << current << std::endl;
		// }
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		if (list_lock_locked)
		{
			if (first != nullptr)
			{
				// there is a first element which we might want to remove
				if (first->value == v)
				{
					// we want to remove the first element
					node_tatas<T> *node_to_remove = first;
					first = first->next;
					delete node_to_remove; // dont need to unlock the mutex since we are deleting the node. No other thread should have access to it.
				}
				else
				{
					// if first value exists but is not the value we want to remove
					first->tatas_lock.unlock();
				}
			}
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			// out.unlock();
			list_lock.unlock();
		}
		// if we reach the end of the list.
		else if (current == nullptr)
		{
			// current is not locked, but previous may be locked
			if (pred != nullptr)
			{
				pred->tatas_lock.unlock();
			}
		}
		else
		{
			// we are in the middle of the list, and we might have found the element we want to remove
			// current and previous is locked. list_lock is not locked
			// out.lock();
			// std::cout << pthread_self() << " current->value: " << current->value << std::endl;
			// std::cout << pthread_self() << " v: " << v << std::endl;
			// std::cout << pthread_self() << " pred->value: " << pred->value << std::endl;
			// out.unlock();
			if (current->value == v)
			{
				// out.lock();
				// std::cout << pthread_self() << " deleting current with value: " << current->value << std::endl;
				// out.unlock();
				// we want to remove current
				pred->next = current->next;
				delete current;
			}
			else
			{
				current->tatas_lock.unlock();
			}
			pred->tatas_lock.unlock();
		}
	}

	/* count elements with value v in the list */
	std::size_t count(T v)
	{
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting count, trying to count value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in count" << std::endl;
		// out.unlock();
		list_lock.lock();
		// out.lock();
		// std::cout << pthread_self() << " list locked in count" << std::endl;
		// out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;

		if (first != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first in count, value: " << first->value << std::endl;
			// out.unlock();
			first->tatas_lock.lock();
			// out.lock();
			// std::cout << pthread_self() << " first locked in count, value: " << first->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl << std::endl;
			// out.unlock();
		}

		node_tatas<T> *pred = nullptr;
		node_tatas<T> *current = first;

		// out.lock();
		// std::cout << pthread_self() << " before while loop in count" << std::endl;
		// out.unlock();

		while (current != nullptr && current->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->tatas_lock.unlock();
			}

			if (list_lock_locked)
			{
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}

			pred = current;

			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				current->tatas_lock.lock();
			}
		}

		// out.lock();
		// std::cout << std::endl << pthread_self() << " after while loop in count" << std::endl;
		//
		// if (current != nullptr)
		// {
		// 	std::cout << pthread_self() << " current->value: " << current->value << std::endl;
		// }
		// else {
		// 	std::cout << pthread_self() << " current: " << current << std::endl;
		// }
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		if (current == nullptr)
		{
			if (list_lock_locked)
			{
				// we never entered the while loop, list is empty or the first element is greater than or equal to the value we are looking for
				list_lock.unlock();
			}
			else if (current == nullptr)
			{
				// we have reached the end of the list
				if (pred != nullptr)
				{
					pred->tatas_lock.unlock();
				}
			}
			return 0;
		}

		std::size_t counter = 0;

		// now we start counting
		// current and pred is locked
		while (current != nullptr && current->value == v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();
				pred->tatas_lock.unlock();
			}
			if (list_lock_locked) // if we wanna count instances of the first element, list_lock will be locked.
			{
				list_lock.unlock();
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}
			counter++;
			pred = current;
			current = current->next;
			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				current->tatas_lock.lock();
			}
		}

		// unlock locks if any are still locked, as we are done counting. this is kinda dogwater but works
		if (current != nullptr)
		{
			current->tatas_lock.unlock();
		}
		if (pred != nullptr)
		{
			pred->tatas_lock.unlock();
		}
		if (list_lock_locked)
		{
			list_lock.unlock();
		}
		return counter;
	}

private:
	node_tatas<T> *first = nullptr;
	TATASLock list_lock;
	std::mutex out;
};

template <typename T>
class fg_clh_sorted_list : public list_superclass<T>
{ // fg = fine grained
public:
	fg_clh_sorted_list() = default;
	fg_clh_sorted_list(const fg_clh_sorted_list<T> &other) = default;
	fg_clh_sorted_list(fg_clh_sorted_list<T> &&other) = default;
	fg_clh_sorted_list<T> &operator=(const fg_clh_sorted_list<T> &other) = default;
	fg_clh_sorted_list<T> &operator=(fg_clh_sorted_list<T> &&other) = default;
	~fg_clh_sorted_list()
	{
		node_clh<T> *current = first;
		while (current != nullptr)
		{
			node_clh<T> *next = current->next;
			delete current;
			current = next;
		}
	}
	void insert(T v)
	{
		// Create QNodes for list_lock
		QNode* list_myNode = new QNode();
		QNode* list_myPred = nullptr;

		// Variables for pred and succ locks
		QNode* pred_myNode = nullptr;
		QNode* pred_myPred = nullptr;

		QNode* succ_myNode = nullptr;
		QNode* succ_myPred = nullptr;

		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting insert, trying to insert value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in insert" << std::endl;
		// out.unlock();

		// Lock the list
		list_lock.lock(list_myNode, list_myPred);

		// out.lock();
		// std::cout << pthread_self() << " list locked in insert" << std::endl;
		// out.unlock();

		// Flag to indicate if list_lock is still held
		bool list_lock_locked = true;

		node_clh<T>* pred = nullptr;
		node_clh<T>* succ = first;

		if (succ != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first node in insert, value: " << succ->value << std::endl;
			// out.unlock();

			// Lock the first node
			succ_myNode = new QNode();
			succ->clh_lock.lock(succ_myNode, succ_myPred);

			// out.lock();
			// std::cout << pthread_self() << " first node locked in insert, value: " << succ->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl;
			// out.unlock();
		}

		// out.lock();
		// std::cout << pthread_self() << " before while loop in insert" << std::endl;
		// out.unlock();

		// Traverse the list to find the correct position
		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " unlocking pred node: " << pred->value << std::endl;
				// out.unlock();

				// Unlock the previous node
				pred->clh_lock.unlock(pred_myNode, pred_myPred);
				delete pred_myNode;
				pred_myNode = nullptr;
				pred_myPred = nullptr;
			}

			if (list_lock_locked)
			{
				// Unlock the list lock
				list_lock.unlock(list_myNode, list_myPred);
				delete list_myNode;
				list_myNode = nullptr;
				list_myPred = nullptr;

				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();

				list_lock_locked = false;
			}

			// Move to the next node
			pred = succ;
			pred_myNode = succ_myNode;
			pred_myPred = succ_myPred;

			succ = succ->next;

			if (succ != nullptr)
			{
				// Lock the next successor node
				succ_myNode = new QNode();
				succ->clh_lock.lock(succ_myNode, succ_myPred);

				// out.lock();
				// std::cout << pthread_self() << " locking succ node: " << succ->value << std::endl;
				// out.unlock();
			}
			else
			{
				succ_myNode = nullptr;
				succ_myPred = nullptr;
			}
		}

		// Now, either pred or list_lock is held, and succ may or may not be locked
		// out.lock();
		// std::cout << pthread_self() << " found position to insert" << std::endl;
		// out.unlock();

		// Create the new node
		node_clh<T>* new_node = new node_clh<T>();
		new_node->value = v;

		if (list_lock_locked)
		{
			// Insert at the beginning of the list
			new_node->next = succ;
			first = new_node;

			if (succ != nullptr)
			{
				// Unlock the successor node
				succ->clh_lock.unlock(succ_myNode, succ_myPred);
				delete succ_myNode;
				succ_myNode = nullptr;
				succ_myPred = nullptr;
			}

			// Unlock the list lock
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in insert" << std::endl;
			// out.unlock();

			list_lock.unlock(list_myNode, list_myPred);
			delete list_myNode;
			list_myNode = nullptr;
			list_myPred = nullptr;
		}
		else if (pred != nullptr && succ == nullptr)
		{
			// Insert at the end of the list
			pred->next = new_node;
			new_node->next = nullptr;

			// Unlock the predecessor node
			pred->clh_lock.unlock(pred_myNode, pred_myPred);
			delete pred_myNode;
			pred_myNode = nullptr;
			pred_myPred = nullptr;
		}
		else if (pred != nullptr && succ != nullptr)
		{
			// Insert in the middle of the list
			pred->next = new_node;
			new_node->next = succ;

			// Unlock successor and predecessor nodes
			succ->clh_lock.unlock(succ_myNode, succ_myPred);
			pred->clh_lock.unlock(pred_myNode, pred_myPred);

			delete succ_myNode;
			succ_myNode = nullptr;
			succ_myPred = nullptr;

			delete pred_myNode;
			pred_myNode = nullptr;
			pred_myPred = nullptr;
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " Error in insert" << std::endl;
			// out.unlock();
			exit(1);
		}

		// Clean up the list lock nodes if they haven't been deleted
		if (list_myNode)
		{
			list_lock.unlock(list_myNode, list_myPred);
			delete list_myNode;
			list_myNode = nullptr;
			list_myPred = nullptr;
		}
	}


	void remove(T v)
	{
		// Create QNodes for list_lock
		QNode* list_myNode = new QNode();
		QNode* list_myPred = nullptr;

		// Variables for pred and curr locks
		QNode* pred_myNode = nullptr;
		QNode* pred_myPred = nullptr;

		QNode* curr_myNode = nullptr;
		QNode* curr_myPred = nullptr;

		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting remove, trying to remove value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in remove" << std::endl;
		// out.unlock();

		// Lock the list
		list_lock.lock(list_myNode, list_myPred);

		// out.lock();
		// std::cout << pthread_self() << " list locked in remove" << std::endl;
		// out.unlock();

		// Flag to indicate if list_lock is still held
		bool list_lock_locked = true;

		node_clh<T>* pred = nullptr;
		node_clh<T>* current = first;

		if (current != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first node in remove, value: " << current->value << std::endl;
			// out.unlock();

			// Lock the first node
			curr_myNode = new QNode();
			current->clh_lock.lock(curr_myNode, curr_myPred);

			// out.lock();
			// std::cout << pthread_self() << " first node locked in remove, value: " << current->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl;
			// out.unlock();

			// Unlock the list lock before returning
			list_lock.unlock(list_myNode, list_myPred);
			delete list_myNode;
			return;
		}

		// out.lock();
		// std::cout << std::endl << pthread_self() << " before while loop in remove" << std::endl;
		// out.unlock();

		// Traverse the list to find the node to remove
		while (current != nullptr && current->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();

				// Unlock the predecessor node
				pred->clh_lock.unlock(pred_myNode, pred_myPred);
				delete pred_myNode;
				pred_myNode = nullptr;
				pred_myPred = nullptr;
			}

			if (list_lock_locked)
			{
				// Unlock the list lock
				list_lock.unlock(list_myNode, list_myPred);
				delete list_myNode;
				list_myNode = nullptr;
				list_myPred = nullptr;

				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();

				list_lock_locked = false;
			}

			// Move to the next node
			pred = current;
			pred_myNode = curr_myNode;
			pred_myPred = curr_myPred;

			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();

				// Lock the current node
				curr_myNode = new QNode();
				current->clh_lock.lock(curr_myNode, curr_myPred);
			}
			else
			{
				curr_myNode = nullptr;
				curr_myPred = nullptr;
			}
		}

		// out.lock();
		// std::cout << std::endl << pthread_self() << " after while loop in remove" << std::endl;

		// if (current != nullptr)
		// {
		// 	std::cout << pthread_self() << " current->value: " << current->value << std::endl;
		// }
		// else
		// {
		// 	std::cout << pthread_self() << " current is nullptr" << std::endl;
		// }
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		if (list_lock_locked)
		{
			if (current != nullptr)
			{
				// There is a first element which we might want to remove
				if (current->value == v)
				{
					// We want to remove the first element
					node_clh<T>* node_to_remove = current;
					first = current->next;

					// Unlock and delete the current node
					node_to_remove->clh_lock.unlock(curr_myNode, curr_myPred);
					delete curr_myNode;
					delete node_to_remove;
					curr_myNode = nullptr;
					curr_myPred = nullptr;
				}
				else
				{
					// If first value exists but is not the value we want to remove
					current->clh_lock.unlock(curr_myNode, curr_myPred);
					delete curr_myNode;
					curr_myNode = nullptr;
					curr_myPred = nullptr;
				}
			}
			// Unlock the list lock
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			// out.unlock();

			list_lock.unlock(list_myNode, list_myPred);
			delete list_myNode;
			list_myNode = nullptr;
			list_myPred = nullptr;
		}
		else if (current == nullptr)
		{
			// We have reached the end of the list
			if (pred != nullptr)
			{
				// Unlock the predecessor node
				pred->clh_lock.unlock(pred_myNode, pred_myPred);
				delete pred_myNode;
				pred_myNode = nullptr;
				pred_myPred = nullptr;
			}
		}
		else
		{
			// We are in the middle of the list, and we might have found the element we want to remove
			// current and pred are locked
			// out.lock();
			// std::cout << pthread_self() << " current->value: " << current->value << std::endl;
			// std::cout << pthread_self() << " v: " << v << std::endl;
			// std::cout << pthread_self() << " pred->value: " << pred->value << std::endl;
			// out.unlock();

			if (current->value == v)
			{
				// out.lock();
				// std::cout << pthread_self() << " deleting current with value: " << current->value << std::endl;
				// out.unlock();

				// Remove current node from the list
				pred->next = current->next;

				// Unlock and delete the current node
				current->clh_lock.unlock(curr_myNode, curr_myPred);
				delete curr_myNode;
				delete current;
				curr_myNode = nullptr;
				curr_myPred = nullptr;
			}
			else
			{
				// Unlock current node
				current->clh_lock.unlock(curr_myNode, curr_myPred);
				delete curr_myNode;
				curr_myNode = nullptr;
				curr_myPred = nullptr;
			}

			// Unlock and delete the predecessor node
			pred->clh_lock.unlock(pred_myNode, pred_myPred);
			delete pred_myNode;
			pred_myNode = nullptr;
			pred_myPred = nullptr;
		}
	}


	std::size_t count(T v)
	{
		// Create QNodes for list_lock
		QNode* list_myNode = new QNode();
		QNode* list_myPred = nullptr;

		// Variables for pred and curr locks
		QNode* pred_myNode = nullptr;
		QNode* pred_myPred = nullptr;

		QNode* curr_myNode = nullptr;
		QNode* curr_myPred = nullptr;

		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting count, trying to count value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in count" << std::endl;
		// out.unlock();

		// Lock the list
		list_lock.lock(list_myNode, list_myPred);

		// out.lock();
		// std::cout << pthread_self() << " list locked in count" << std::endl;
		// out.unlock();

		// Flag to indicate if list_lock is still held
		bool list_lock_locked = true;

		node_clh<T>* pred = nullptr;
		node_clh<T>* current = first;

		if (current != nullptr)
		{
			// out.lock();
			// std::cout << pthread_self() << " locking first node in count, value: " << current->value << std::endl;
			// out.unlock();

			// Lock the first node
			curr_myNode = new QNode();
			current->clh_lock.lock(curr_myNode, curr_myPred);

			// out.lock();
			// std::cout << pthread_self() << " first node locked in count, value: " << current->value << std::endl;
			// out.unlock();
		}
		else
		{
			// out.lock();
			// std::cout << pthread_self() << " list is empty" << std::endl;
			// out.unlock();

			// Unlock the list lock before returning
			list_lock.unlock(list_myNode, list_myPred);
			delete list_myNode;
			return 0;
		}

		// out.lock();
		// std::cout << pthread_self() << " before while loop in count" << std::endl;
		// out.unlock();

		// Traverse the list to find the starting point for counting
		while (current != nullptr && current->value < v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();

				// Unlock the predecessor node
				pred->clh_lock.unlock(pred_myNode, pred_myPred);
				delete pred_myNode;
				pred_myNode = nullptr;
				pred_myPred = nullptr;
			}

			if (list_lock_locked)
			{
				// Unlock the list lock
				list_lock.unlock(list_myNode, list_myPred);
				delete list_myNode;
				list_myNode = nullptr;
				list_myPred = nullptr;

				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();

				list_lock_locked = false;
			}

			// Move to the next node
			pred = current;
			pred_myNode = curr_myNode;
			pred_myPred = curr_myPred;

			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();

				// Lock the current node
				curr_myNode = new QNode();
				current->clh_lock.lock(curr_myNode, curr_myPred);
			}
			else
			{
				curr_myNode = nullptr;
				curr_myPred = nullptr;
			}
		}

		// out.lock();
		// std::cout << std::endl << pthread_self() << " after while loop in count" << std::endl;
		// if (current != nullptr)
		// {
		// 	std::cout << pthread_self() << " current->value: " << current->value << std::endl;
		// }
		// else
		// {
		// 	std::cout << pthread_self() << " current is nullptr" << std::endl;
		// }
		// std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		// out.unlock();

		if (current == nullptr || current->value > v)
		{
			// Value not found in the list
			if (curr_myNode != nullptr)
			{
				current->clh_lock.unlock(curr_myNode, curr_myPred);
				delete curr_myNode;
			}
			if (pred_myNode != nullptr)
			{
				pred->clh_lock.unlock(pred_myNode, pred_myPred);
				delete pred_myNode;
			}
			if (list_lock_locked)
			{
				list_lock.unlock(list_myNode, list_myPred);
				delete list_myNode;
			}
			return 0;
		}

		// Now, current->value == v
		std::size_t counter = 0;

		// Start counting the occurrences of v
		while (current != nullptr && current->value == v)
		{
			if (pred != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) unlocking pred: " << pred->value << std::endl;
				// out.unlock();

				// Unlock the predecessor node
				pred->clh_lock.unlock(pred_myNode, pred_myPred);
				delete pred_myNode;
				pred_myNode = nullptr;
				pred_myPred = nullptr;
			}

			if (list_lock_locked)
			{
				// Unlock the list lock
				list_lock.unlock(list_myNode, list_myPred);
				delete list_myNode;
				list_myNode = nullptr;
				list_myPred = nullptr;

				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();

				list_lock_locked = false;
			}

			// Increment the counter
			counter++;

			// Move to the next node
			pred = current;
			pred_myNode = curr_myNode;
			pred_myPred = curr_myPred;

			current = current->next;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) locking current: " << current->value << std::endl;
				// out.unlock();

				// Lock the current node
				curr_myNode = new QNode();
				current->clh_lock.lock(curr_myNode, curr_myPred);
			}
			else
			{
				curr_myNode = nullptr;
				curr_myPred = nullptr;
			}
		}

		// Unlock any remaining locks
		if (curr_myNode != nullptr)
		{
			current->clh_lock.unlock(curr_myNode, curr_myPred);
			delete curr_myNode;
		}
		if (pred_myNode != nullptr)
		{
			pred->clh_lock.unlock(pred_myNode, pred_myPred);
			delete pred_myNode;
		}
		if (list_lock_locked)
		{
			list_lock.unlock(list_myNode, list_myPred);
			delete list_myNode;
		}

		return counter;
	}


private:
	node_clh<T> *first = nullptr;
	std::mutex out;	
	CLHLock2 list_lock; // this locks new threads from accesing the list
};