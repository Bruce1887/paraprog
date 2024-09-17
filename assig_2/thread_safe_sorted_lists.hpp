#pragma once

/*
 * This code was originally written by David Klaftenegger in 2015.
 * It was modified by Edvin Bruce and Viktor Wallsten in 2024 as part of an assignment.
 */

#include <mutex>
#include <atomic>

class TATASLock {
private:
    std::atomic<bool> locked{false};  // Initially unlocked

public:
    void lock() {
        while (true) {
            //Wait until lock looks free
            while (locked.load()) {
                //Spin here until it might become unlocked
            }
            //try to take the lock
            if (!locked.exchange(true)) {
                break;  // Lock acquired successfully
            }
        }
    }

    void unlock() {
        locked.store(false); //Unlock the lock
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
	fg_mutex_sorted_list() = default;
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
		//idea: you go in a kind of "hand in hand" way, so first lock the first node, lock the next and move on, and unlock the previous and so on.

		/* first find position */
		node_mutex<T> *pred = nullptr;
		node_mutex<T> *succ = first;

		if (succ != nullptr){
			succ->mutex.lock(); //lock the first node
		}

		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				pred->mutex.unlock(); //unlock previous node
			}

			pred = succ; //move along
            succ = succ->next;

			if (succ != nullptr)
            {
                succ->mutex.lock(); // Lock the next node.
            }
		}

		/* construct new node */
		node_mutex<T> *current = new node_mutex<T>();
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
		if (succ != nullptr) //when we have inserted our current we need to unlock prev and succ
        {
            succ->mutex.unlock();
        }
        if (pred != nullptr)
        {
            pred->mutex.unlock();
        }
	}

	void remove(T v)
    {
        node_mutex<T> *pred = nullptr;
        node_mutex<T> *current = first;

        if (current != nullptr) {
            current->mutex.lock(); // Lock the first node.
        }

        while (current != nullptr && current->value < v) // iterate the same as in the insert way
        {
            if (pred != nullptr)
            {
                pred->mutex.unlock(); 
            }

            pred = current;
            current = current->next;

            if (current != nullptr)
            {
                current->mutex.lock();
            }
        }

        if (current == nullptr || current->value != v)
        {
            if (current != nullptr)
            {
                current->mutex.unlock();
            }
            if (pred != nullptr)
            {
                pred->mutex.unlock();
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

        current->mutex.unlock();
        delete current;

        if (pred != nullptr)
        {
            pred->mutex.unlock();
        }
    }
	/* count elements with value v in the list */
	std::size_t count(T v)
	{
		std::size_t cnt = 0;
		/* first go to value v */
		node_mutex<T> *current = first;
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
	node_mutex<T> *first = nullptr;
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
		//idea: you go in a kind of "hand in hand" way, so first lock the first node, lock the next and move on, and unlock the previous and so on.

		/* first find position */
		node_tatas<T> *pred = nullptr;
		node_tatas<T> *succ = first;

		if (succ != nullptr){
			succ->tatas_lock.lock(); //lock the first node
		}

		while (succ != nullptr && succ->value < v)
		{
			if (pred != nullptr)
			{
				pred->tatas_lock.unlock(); //unlock previous node
			}

			pred = succ; //move along
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
		if (succ != nullptr) //when we have inserted our current we need to unlock prev and succ
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

        if (current != nullptr) {
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
