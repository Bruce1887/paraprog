#pragma once

/*
 * This code was originally written by David Klaftenegger in 2015.
 * It was modified by Edvin Bruce and Viktor Wallsten in 2024 as part of an assignment.
 */

#include <mutex>
#include <atomic>
#include <functional>
#include <pthread.h>


class CLHLock2
{
private:
	std::atomic<bool*> tail;
	std::mutex out;

public:
	CLHLock2()
	{
		tail.store(new bool{false});
	}
	~CLHLock2() = default;

	bool *lock()
	{		
		bool* thread_local_bool = new bool{true};
		out.lock();
		std::cout << pthread_self() << " thread is trying to lock" << std::endl;
		std::cout << "tail: " << tail.load() << "-" << *tail.load() << std::endl;
		std::cout << " thread_local_bool: " << thread_local_bool << "-" << *thread_local_bool << std::endl;
		out.unlock();

		bool* old_bool = tail.exchange(thread_local_bool);

		out.lock();
		std::cout << "old_bool: "<< old_bool << "-" << *old_bool<< std::endl;
		out.unlock();

		// tail is now set to the thread_local_bools value
		// old bool is the bool this thread should spin on
		tail = thread_local_bool;

		out.lock();
		std::cout << pthread_self() << " starting busy-wait" << std::endl;
		out.unlock();
		int local_wait_counter = 0;
		while (*old_bool)
		{
			local_wait_counter++;
			// Busy-wait
			// out.lock();
			// std::cout << pthread_self() << " thread is spinning" << std::endl;
			// out.unlock();
			
		}		
		// Free old bool?
		delete old_bool;

		out.lock();
		std::cout << pthread_self() << " exited busy-wait after " << local_wait_counter << " cycles." << std::endl;
		out.unlock();
		return thread_local_bool;
	}

	void unlock(bool *tl_bool)
	{ // this is the bool that was returned by aquire
		if (tl_bool)
		{	
			out.lock();
			std::cout << "unlocking CLH lock" << std::endl;
			std::cout << "thread_local_bool before setting value to false: " << tl_bool << "-" << *tl_bool << std::endl;
			*tl_bool = false;
			std::cout << "thread_local_bool after setting value to false: " << tl_bool << "-" << *tl_bool << std::endl;
			std::cout << "tail after setting value to false: " << tail << "-" << *tail << std::endl;
			out.unlock();
		}
		else
		{
			std::cout << pthread_self() << " thread tried to unlock a nullptr!!" << std::endl;
			exit(1);
		}
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
		bool *list_bool;
		bool *pred_bool;
		bool *succ_bool;

		out.lock();
		std::cout << std::endl << pthread_self() << " starting insert, trying to insert value: " << v << std::endl;
		std::cout << pthread_self() << " locking list in insert" << std::endl;
		out.unlock();
		list_bool = list_lock.lock();
		out.lock();
		std::cout << pthread_self() << " list locked in insert" << std::endl;
		out.unlock();

		// we use this bool to have each thread only unlock the list_lock once
		bool list_lock_locked = true;
		if (first != nullptr)
		{
			out.lock();
			std::cout << pthread_self() << " locking first in insert, value: " << first->value << std::endl;
			out.unlock();
			succ_bool = first->clh_lock.lock();
			out.lock();
			std::cout << pthread_self() << " first locked in insert, value: " << first->value << std::endl;
			out.unlock();
		}
		else
		{
			out.lock();
			std::cout << pthread_self() << " list is empty" << std::endl << std::endl;
			out.unlock();
		}

		node_clh<T> *pred = nullptr;
		node_clh<T> *succ = first;

		out.lock();
		std::cout << pthread_self() << " before while loop in insert" << std::endl;
		out.unlock();

		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				out.lock();
				std::cout << pthread_self() << " (in while-loop) unlocking pred: " << pred->value << std::endl;
				out.unlock();
				pred->clh_lock.unlock(pred_bool);
			}

			if (list_lock_locked)
			{
				list_lock.unlock(list_bool);
				out.lock();
				std::cout << pthread_self() << " list is unlocked" << std::endl;
				out.unlock();
				list_lock_locked = false;
			}

			pred = succ;
			pred_bool = succ_bool;

			succ = succ->next;
			succ_bool = nullptr; // maybe this is not null, but that cant be determined until the if statement just after this

			if (succ != nullptr)
			{
				out.lock();
				std::cout << pthread_self() << " (in while-loop) locking succ: " << succ->value << std::endl;
				out.unlock();
				succ_bool = succ->clh_lock.lock();
			}
		}
		// previous or list_lock is now locked (if we are at the beginning of the list)
		// succ may be locked or is nullptr (if we are at end of list)
		out.lock();
		std::cout << pthread_self() << " succ: " << succ << std::endl;
		std::cout << pthread_self() << " list_locked: " << list_lock_locked << std::endl;
		out.unlock();

		node_clh<T> *new_node = new node_clh<T>();
		new_node->value = v;

		if (list_lock_locked)
		{
			// if the list_lock is still locked, we should insert the new node at the beginning of the list
			new_node->next = succ;
			first = new_node;
			if (new_node->next != nullptr)
			{
				new_node->next->clh_lock.unlock(succ_bool);
			}
			out.lock();
			std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			out.unlock();
			list_lock.unlock(list_bool);
		}
		else if (pred != nullptr && succ == nullptr)
		{
			// if we are at the end of the list. Now only pred is locked
			pred->next = new_node;
			new_node->next = nullptr;
			pred->clh_lock.unlock(pred_bool);
		}
		else if (pred != nullptr && succ != nullptr)
		{
			// if we are in the middle of the list. Now pred and succ is locked
			pred->next = new_node;
			new_node->next = succ;
			succ->clh_lock.unlock(succ_bool);
			pred->clh_lock.unlock(pred_bool);
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
		bool *list_bool;
		bool *pred_bool;
		bool *curr_bool;

		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting remove, trying to remove value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in remove" << std::endl;
		// out.unlock();
		list_bool = list_lock.lock();
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
			curr_bool = first->clh_lock.lock();
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

		node_clh<T> *pred = nullptr;
		node_clh<T> *current = first;

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
				pred->clh_lock.unlock(pred_bool);
			}

			if (list_lock_locked)
			{
				list_lock_locked = false;
				list_lock.unlock(list_bool);
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
			}

			pred = current;
			pred_bool = curr_bool;

			current = current->next;
			curr_bool = nullptr; // maybe this is not null, but that cant be determined until the if statement just after this

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				curr_bool = current->clh_lock.lock();
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
			if (current != nullptr)
			{
				// there is a first element which we might want to remove
				if (current->value == v)
				{
					// we want to remove the first element
					node_clh<T> *node_to_remove = current;
					first = current->next;
					delete node_to_remove; // dont need to unlock the mutex since we are deleting the node. No other thread should have access to it.
				}
				else
				{
					// if first value exists but is not the value we want to remove
					current->clh_lock.unlock(curr_bool);
				}
			}
			// out.lock();
			// std::cout << pthread_self() << " unlocking list in remove" << std::endl;
			// out.unlock();
			list_lock.unlock(list_bool);
		}
		// if we reach the end of the list.
		else if (current == nullptr)
		{
			// current is not locked, but previous may be locked
			if (pred != nullptr)
			{
				pred->clh_lock.unlock(pred_bool);
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
				current->clh_lock.unlock(curr_bool);
			}
			pred->clh_lock.unlock(pred_bool);
		}
	}

	std::size_t count(T v)
	{
		bool *list_bool;
		bool *pred_bool;
		bool *curr_bool;
		// out.lock();
		// std::cout << std::endl << pthread_self() << " starting count, trying to count value: " << v << std::endl;
		// std::cout << pthread_self() << " locking list in count" << std::endl;
		// out.unlock();
		list_bool = list_lock.lock();
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
			curr_bool = first->clh_lock.lock();
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

		node_clh<T> *pred = nullptr;
		node_clh<T> *current = first;

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
				pred->clh_lock.unlock(pred_bool);
			}

			if (list_lock_locked)
			{
				list_lock.unlock(list_bool);
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}

			pred = current;
			pred_bool = curr_bool;

			current = current->next;
			curr_bool = nullptr; // maybe this is not null, but that cant be determined until the if statement just after this

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				curr_bool = current->clh_lock.lock();
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
				list_lock.unlock(list_bool);
			}
			else if (current == nullptr)
			{
				// we have reached the end of the list
				if (pred != nullptr)
				{
					pred->clh_lock.unlock(pred_bool); 
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
				pred->clh_lock.unlock(pred_bool);
			}
			if (list_lock_locked) // if we wanna count instances of the first element, list_lock will be locked.
			{
				list_lock.unlock(list_bool);
				// out.lock();
				// std::cout << pthread_self() << " list is unlocked" << std::endl;
				// out.unlock();
				list_lock_locked = false;
			}
			counter++;

			pred = current;
			pred_bool = curr_bool;
			
			current = current->next;			
			curr_bool = nullptr;

			if (current != nullptr)
			{
				// out.lock();
				// std::cout << pthread_self() << " (in counting while-loop) locking current: " << current->value << std::endl;
				// out.unlock();
				curr_bool = current->clh_lock.lock();
			}
		}

		// unlock locks if any are still locked, as we are done counting. this is kinda dogwater but works
		if (current != nullptr)
		{
			current->clh_lock.unlock(curr_bool);
		}
		if (pred != nullptr)
		{
			pred->clh_lock.unlock(pred_bool);
		}
		if (list_lock_locked)
		{
			list_lock.unlock(list_bool);
		}
		return counter;
	}

private:
	node_clh<T> *first = nullptr;
	std::mutex out;	
	CLHLock2 list_lock; // this locks new threads from accesing the list
};