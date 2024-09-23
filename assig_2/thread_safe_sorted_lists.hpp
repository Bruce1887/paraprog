#pragma once

/*
 * This code was originally written by David Klaftenegger in 2015.
 * It was modified by Edvin Bruce and Viktor Wallsten in 2024 as part of an assignment.
 */

#include <mutex>
#include <atomic>
#include <functional>

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
class list_superclass
{
public:
	virtual void insert(T v) = 0;
	virtual void remove(T v) = 0;
	virtual std::size_t count(T v) = 0;
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
	fg_mutex_sorted_list() : list_superclass<T>(){
		node_mutex<T> *dummy = new node_mutex<T>();
		dummy->next = nullptr;
	    dummy->value = std::numeric_limits<int>::max();
		first = dummy;
		std::cout << "fg_mutex_sorted_list created with dummy" << std::endl;
	}
	fg_mutex_sorted_list(const fg_mutex_sorted_list<T> &other) = default;
	fg_mutex_sorted_list(fg_mutex_sorted_list<T> &&other) = default;
	fg_mutex_sorted_list<T> &operator=(const fg_mutex_sorted_list<T> &other) = default;
	fg_mutex_sorted_list<T> &operator=(fg_mutex_sorted_list<T> &&other) = default;
	~fg_mutex_sorted_list()
	{
		while (first != nullptr)
		{
			remove(first->value);
		}
	}
	void insert(T v)
	{
		auto predicate = [v](node_mutex<T>* n) {return n->value >= v;}; 
		node_mutex<T>* previous = find_first_by_predicate(predicate);

		node_mutex<T>* new_node = new node_mutex<T>();
		new_node->value = v;

		std::cout << "value to insert: " << v << std::endl;
		if(previous != nullptr) {
			std::cout << "value found from predicate: "<< previous->value << ", next: " << previous->next->value << std::endl;
			new_node->next = previous->next;
			previous->next = new_node;
			previous->next->mutex.unlock();
			previous->mutex.unlock();
		}
		else {
			std::cout << "no value found from predicate"<< std::endl;
			std::cout << "first->val: "<< first->value << std::endl;
			// current is null, this means that the list only has a dummy node (in this case)
			bool free_first_when_done = first->next == nullptr;
			new_node->next = first;
			first = new_node;
			new_node->next->mutex.unlock();
			std::cout << "added node with value: "<< new_node->value << std::endl;
			std::cout << "free_first_when_done: "<< free_first_when_done << std::endl;
			if (free_first_when_done) {
				first_lock.unlock();
			}
		}
	}

	void remove(T v)
	{	
		// fetches the node before the node with value v. Locks the node before the node with value v and the node with value v (if it finds it).
		auto predicate = [v](node_mutex<T>* n) {return n->next != nullptr && n->next->value == v;}; 
		node_mutex<T>* prev = find_first_by_predicate(predicate);  
		if (prev != nullptr) {
			//prev is not null and is locked
			if (prev->next != nullptr) {
				//prev->next is not null and is locked

				//delete prev->next and relink the list
				auto node_to_delete = prev->next;
				prev->next = node_to_delete->next;
				delete node_to_delete;				

				//unlock prev->next	
				prev->next->mutex.unlock();
			}
			//unlock prev
			prev->mutex.unlock();
		}
	}

	std::size_t count(T v)
	{
		node_mutex<T> *current = first; // first is never null

		std::cout << "locking first (current)" << std::endl;
		current->mutex.lock();
		std::cout << "first locked (current)" << std::endl;

		while (current != nullptr)
		{	
			std::cout << "current->value: " << current->value << std::endl;
			if (current->next != nullptr)
			{
				std::cout << "locking current->next with val: " << current->next->value <<  std::endl;
				current->next->mutex.lock();
				std::cout << "current->next locked" << std::endl;
			}
			// current and current->next is now locked, if they are not null
			if (current->value == v) break;
			
			// unlock current but not current->next
			current->mutex.unlock();

			// set current to current->next
			current = current->next;			
		}

		if (current == nullptr) return 0; // if we did not find a value v
		
		std::size_t cnt = 0;
		while (current != nullptr && current->value == v)
		{
			if (current->next != nullptr)
			{
				current->next->mutex.lock();
			}

			// count
			cnt++;
			std::cout << "counting: " << cnt << std::endl;

			// unlock current but not current->next
			current->mutex.unlock();

			// set current to current->next
			current = current->next;			
		}
		return cnt;
	}

private:
	node_mutex<T> *first = nullptr; // this should be a dummy when the list is empty
	std::mutex first_lock; // this locks the first node in the list

	// returns a reference to the first node before the node that satisfies the predicate. 
	// the first that satisfies the predicate node is locked as well as the previous one when this function returns.
	// returns nullptr if no node satisfies the predicate, or if the first node does satisfy the predicate.
	// i.e. if the first node satisfies the predicate, the first node is locked and the function returns nullptr.
	node_mutex<T>* find_first_by_predicate(std::function<bool(node_mutex<T> *)> predicate)
	{
		std::cout << std::endl << "starting find_first_by_predicate"<< std::endl;

		std::cout << "lock first_lock in ffbp..."<< std::endl;
		first_lock.lock();
		std::cout << "first_lock locked in ffbp."<< std::endl;
		node_mutex<T> *previous = nullptr;
		node_mutex<T> *current = first; // first is never null

		std::cout << "lock current in ffbp... "<< current->value << std::endl;
		current->mutex.lock();
		std::cout << "current locked in ffbp."<< current->value << std::endl;

		while (current != nullptr)
		{			
			if (previous != nullptr)
			{
				previous->mutex.unlock();
			}
			else {
				first_lock.unlock();
			}

			previous = current;
			current = current->next;

			if (current != nullptr)
			{
				std::cout << "lock current in ffbp... "<< current->value << std::endl;
				current->mutex.lock();
				std::cout << "current locked in ffbp."<< current->value << std::endl;
				if (predicate(current)) {
					return previous;
				}
			}
			else {
				previous->mutex.unlock();				
			}
		}
		return nullptr;
	}
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
		// idea: you go in a kind of "hand in hand" way, so first lock the first node, lock the next and move on, and unlock the previous and so on.

		/* first find position */
		node_tatas<T> *pred = nullptr;
		node_tatas<T> *succ = first;

		if (succ != nullptr)
		{
			succ->tatas_lock.lock(); // lock the first node
		}

		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				pred->tatas_lock.unlock(); // unlock previous node
			}

			pred = succ; // move along
			succ = succ->next;

			if (succ != nullptr)
			{
				succ->tatas_lock.lock(); // Lock the next node.
			}
		}

		/* construct new node */
		node_tatas<T> *current = new node_tatas<T>();
		current->value = v;
		current->next = succ;

		if (pred == nullptr)
		{
			first = current;
		}
		else
		{
			pred->next = current;
		}
		if (succ != nullptr) // when we have inserted our current we need to unlock prev and succ
		{
			succ->tatas_lock.unlock();
		}
		if (pred != nullptr)
		{
			pred->tatas_lock.unlock();
		}
	}

	void remove(T v)
	{
		node_tatas<T> *pred = nullptr;
		node_tatas<T> *current = first;

		if (current != nullptr)
		{
			current->tatas_lock.lock(); // Lock the first node.
		}

		while (current != nullptr && current->value < v) // iterate the same as in the insert way
		{
			if (pred != nullptr)
			{
				pred->tatas_lock.unlock();
			}

			pred = current;
			current = current->next;

			if (current != nullptr)
			{
				current->tatas_lock.lock();
			}
		}

		if (current == nullptr || current->value != v)
		{
			if (current != nullptr)
			{
				current->tatas_lock.unlock();
			}
			if (pred != nullptr)
			{
				pred->tatas_lock.unlock();
			}
			return; // v not found
		}

		if (pred == nullptr)
		{
			first = current->next;
		}
		else
		{
			pred->next = current->next;
		}

		current->tatas_lock.unlock();
		delete current;

		if (pred != nullptr)
		{
			pred->tatas_lock.unlock();
		}
	}

	/* count elements with value v in the list */
	std::size_t count(T v)
	{
		std::size_t cnt = 0;
		/* first go to value v */
		node_tatas<T> *current = first;
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
		return cnt;
	}

private:
	node_tatas<T> *first = nullptr;
};
