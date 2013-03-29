#pragma once

namespace Reporter {
	class IReporter;
}

namespace Allocator {
	class IAllocator;
}

// ------------------------------------------------------------------------------------------
namespace Utils
{

	// Macro Helpers
#define TLOOP(classtype, list, variable) Utils::TLISTNODE<classtype> *variable = NULL; while (variable = list->GetNextNode(variable))
#define TINVERTEDLOOP(classtype, list, variable) Utils::TLISTNODE<classtype> *variable = NULL; while (variable = list->GetPreviousNode(variable))

	// -------------------------------------------------------------------------------
	template <typename T>
	class TLISTNODE
	{
		// class data holder, doesn't have responsibility for allocate/delete the data
	public:

		TLISTNODE()
			: Data(NULL), Next(NULL), Previous(NULL)
		{};

		T			*Data;
		TLISTNODE	*Next;
		TLISTNODE	*Previous;
	};


	// -------------------------------------------------------------------------------
	template <typename T>
	class TLIST
	{
	protected:
		long		Count; // number of occurancies

		Allocator::IAllocator	*Allocator;
		Reporter::IReporter		*Reporter;

		TLISTNODE<T>	*FirstNode;
		TLISTNODE<T>	*LastNode;

		TLIST(Allocator::IAllocator *allocator, Reporter::IReporter *reporter)
			: Allocator(allocator), Reporter(reporter), Count(0), FirstNode(NULL), LastNode(NULL)
		{
			if (Allocator)
			{
				Allocator->AddRef();
			}
			if (Reporter)
			{
//				Reporter->AddRef();
			}
		};

	public:

		static TLIST		*Create(Allocator::IAllocator *allocator, Reporter::IReporter *reporter);
		void				Destroy();

		void				DestroyAll(); // Destroy content and destroy list
		void				DestroyContent();
		void				DeleteAllAndDestroy();

		// Provide Interfaces
//		inline void		SetIReport(IREPORT *reporter);

		// Add data to the list
		TLISTNODE<T>	*Add(T* data); // by default add to the first one
		TLISTNODE<T>	*AddToLast(T* data); // add at the end
		TLISTNODE<T>	*AddAfter(T* data, TLISTNODE<T>* node);
		TLISTNODE<T>	*AddBefore(T* data, TLISTNODE<T>* node);
		TLISTNODE<T>	*AddSorted(T* data); // your structure need to overload the operator < if you use this function
		TLISTNODE<T>	*AddIfNotInList(T* data);

		// Remove functions only delete the node without deleting the data it holds
		void		Remove(TLISTNODE<T>* node);
		bool		Remove(T* data); // slow version (need to go through all the list until it found the data)
		void		RemoveAll();

		// Delete functions delete the data the node holds as well as the node itself
		void		Delete(TLISTNODE<T>* node);
		void		Delete(T* data); // slow version (need to go through all the list until it found the data)
		void		DeleteAll();

		// Enable to call a function classmember on each node
		typedef  void (T::*Function)();
		void		CallOnAll(Function function);

		TLISTNODE<T>	*GetNextNode(TLISTNODE<T>* node = NULL);
		TLISTNODE<T>	*GetPreviousNode(TLISTNODE<T>* node = NULL);
		TLISTNODE<T>	*GetNode(long index);
		TLISTNODE<T>	*GetNode(T *data);

		TLISTNODE<T>	*IsInList(T* data); // slow because it needs to check all the list first (make sure it's inserted only once and use add instead)
		bool		IsEmpty() {return Count == 0;}

		long		GetSize() {return Count;} // return the number of occurance in the list
	};



	// -------------------------------------------------------------------------------
	template <typename T>
	TLIST<T>		*TLIST<T>::Create(Allocator::IAllocator *allocator, Reporter::IReporter *reporter)
	{
		if (allocator == NULL)
		{
			return new TLIST<T>(allocator, reporter);
		}

		void *memory = allocator->Allocate(sizeof(TLIST<T>), typeid(T).name(), __FILE__, __LINE__); // typeid allows to have the real name for memory leak detection
		return new (memory) TLIST<T>(allocator, reporter);
	}

	// -------------------------------------------------------------------------------
	template <typename T>
	void	TLIST<T>::Destroy()
	{
		// destroy nodes in the list
		RemoveAll();

		/*
		if (Reporter)
		{
			Reporter->DecRef();
		}
		*/

		if (Allocator == NULL)
		{
			delete this;
			return;
		}

		Allocator::IAllocator *alloc = Allocator;

		this->~TLIST();
		Allocator->Free(this);

		alloc->DecRef();
	}

	// -------------------------------------------------------------------------------
	template <typename T>
	void	TLIST<T>::DestroyAll()
	{
		DestroyContent();
		Destroy();
	}

	// -------------------------------------------------------------------------------
	template <typename T>
	void	TLIST<T>::DestroyContent()
	{
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			node->Data->Destroy();
		}

		RemoveAll();
	}


	// -------------------------------------------------------------------------------
	template <typename T>
	void	TLIST<T>::DeleteAllAndDestroy()
	{
		DeleteAll();
		Destroy();
	}

	// -------------------------------------------------------------------------------------------------
	template <typename T>
	void	TLIST<T>::CallOnAll(Function function)
	{
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			(node->Data->*(function))();
		}
	}


	// -------------------------------------------------------------------------------
	/*
	template <typename T>
	void	TLIST<T>::SetIReport(IREPORT *reporter)
	{
		if (Reporter)
			Reporter->DecRef();

		Reporter = reporter;

		if (Reporter)
			Reporter->AddRef();
	}
	*/

	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>	*TLIST<T>::Add(T* data)
	{ // Insert at the beginning
		TLISTNODE<T>* new_node;
		if (Allocator)
		{
			void *memory = Allocator->Allocate(sizeof(TLISTNODE<T>), typeid(T).name(), __FILE__, __LINE__); // typeid allows to have the real name for memory leak detection
			new_node = new (memory) TLISTNODE<T>();
		}
		else
			new_node = new TLISTNODE<T>();

		if (LastNode == NULL)
			LastNode = new_node;

		new_node->Data = data;
		new_node->Next = FirstNode;
		if (new_node->Next)
			new_node->Next->Previous = new_node;

		new_node->Previous = NULL;

		FirstNode = new_node;

		Count ++;

		return new_node;
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>	*TLIST<T>::AddIfNotInList(T* data)
	{ // Add the not if data not already in list

		if (IsInList(data))
		{ // it's already added
			return NULL;
		}

		// it's not so add it
		return Add(data);
	}


	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>	*TLIST<T>::AddToLast(T* data)
	{ // insert at the end of the list

		TLISTNODE<T> *new_node;
		if (Allocator)
		{
			void *memory = Allocator->Allocate(sizeof(TLISTNODE<T>), typeid(T).name(), __FILE__, __LINE__); // typeid allows to have the real name for memory leak detection
			new_node = new (memory) TLISTNODE<T>();
		}
		else
			new_node = new TLISTNODE<T>();

		if (FirstNode == NULL)
			FirstNode = new_node;

		new_node->Data = data;

		new_node->Next = NULL;
		new_node->Previous = LastNode;
		if (new_node->Previous)
			new_node->Previous->Next = new_node;

		LastNode = new_node;

		Count ++;

		return new_node;
	};

	// ---------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>	*TLIST<T>::AddSorted(T* data)
	{
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			if (*node->Data < *data)
				continue;

			// data is now smaller than the one we looked at, so we insert before the one we look at
			return AddBefore(data, node);
		}

		// our new data is bigger than any other one (or the list is empty), so we add to the end
		return AddToLast(data);
	}


	// ---------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>	*TLIST<T>::AddAfter(T* data, TLISTNODE<T>* node)
	{
		if (node == NULL)
		{
			// empty list add list any element
			return AddToLast(data);
		}

		TLISTNODE<T>* new_node;
		if (Allocator)
		{
			void *memory = Allocator->Allocate(sizeof(TLISTNODE<T>), typeid(T).name(), __FILE__, __LINE__); // typeid allows to have the real name for memory leak detection
			new_node = new (memory) TLISTNODE<T>();
		}
		else
			new_node = new TLISTNODE<T>();


		new_node->Data = data;

		new_node->Next = node->Next;
		new_node->Previous = node;

		if (node->Next)
			node->Next->Previous = new_node;

		node->Next = new_node;

		if (LastNode == node)
			LastNode = new_node;

		Count ++;

		return new_node;
	}

	// ---------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>	*TLIST<T>::AddBefore(T* data, TLISTNODE<T>* node)
	{
		TLISTNODE<T>* new_node;
		if (Allocator)
		{
			void *memory = Allocator->Allocate(sizeof(TLISTNODE<T>), typeid(T).name(), __FILE__, __LINE__); // typeid allows to have the real name for memory leak detection
			new_node = new (memory) TLISTNODE<T>();
		}
		else
			new_node = new TLISTNODE<T>();

		new_node->Data = data;

		if (FirstNode == node)
			FirstNode = new_node;
		else
			node->Previous->Next = new_node;

		new_node->Next = node;
		new_node->Previous = node->Previous;

		node->Previous = new_node;

		Count ++;

		return new_node;
	}


	// -------------------------------------------------------------------------------------------
	template <typename T>
	bool TLIST<T>::Remove(T* data)
	{ // SLOW
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			if (node->Data == data)
			{ // node to destroy
				Remove(node);
				return true;
			}
		}

		if (Reporter)
			Reporter->Report(Reporter::IReporter::eERROR, __FUNCTION__, __FILE__, __LINE__, "[Utils::TLIST] Remove Object not in Utils::TLIST");

		return false;
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	void TLIST<T>::Remove(TLISTNODE<T>* node)
	{
		if (node == NULL) return;

		if (FirstNode == node)
			FirstNode = node->Next;

		if (LastNode == node)
			LastNode = node->Previous;

		if (node->Previous)
			node->Previous->Next = node->Next;

		if (node->Next)
			node->Next->Previous = node->Previous;

		if (Allocator)
		{
			node->~TLISTNODE();
			Allocator->Free(node);
		}
		else
			delete node;

		Count--;
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	void TLIST<T>::RemoveAll()
	{
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(NULL))
		{
			Remove(node->Data);
		}
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	void TLIST<T>::Delete(T* data)
	{ // SLOW
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			if (node->Data == data)
			{ // node and data to destroy
				Delete(node);
				return;
			}
		}

//		if (Reporter)
//			Reporter->Report(REPORTITEM::eERROR, __FILE__, __LINE__, "[Utils::TLIST] Delete Object not in Utils::TLIST");
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	void TLIST<T>::Delete(TLISTNODE<T>* node)
	{
		if (node == NULL) return;

		if (FirstNode == node)
			FirstNode = node->Next;

		if (LastNode == node)
			LastNode = node->Previous;

		if (node->Previous)
			node->Previous->Next = node->Next;

		if (node->Next)
			node->Next->Previous = node->Previous;

		if (Allocator)
		{
			node->Data->~T();
			Allocator->Free(node->Data);

			node->~TLISTNODE();
			Allocator->Free(node);
		}
		else
		{
			delete node->Data;
			delete node;
		}

		Count--;
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	void TLIST<T>::DeleteAll()
	{
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(NULL))
		{
			Delete(node->Data);
		}
	};


	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>* TLIST<T>::IsInList(T* data)
	{ // slow
		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			if (node->Data == data)
			{ 
				return node;
			}
		}
		return NULL;
	};

	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>*   TLIST<T>::GetNextNode(TLISTNODE<T>* node)
	{
		if (Count == 0) return NULL;
		if (node == NULL) return FirstNode; // enable to use this function in a while loop to go through all the occurancies
		return node->Next;
	}

	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>*   TLIST<T>::GetPreviousNode(TLISTNODE<T>* node)
	{
		if (Count == 0) return NULL;
		if (node == NULL) return LastNode; // enable to use this function in a while loop to go through all the occurancies
		return node->Previous;
	}

	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>*	TLIST<T>::GetNode(T *data)
	{ // slow
		if (index >= Count) return NULL;

		TLISTNODE<T>* node = NULL;
		while (node = GetNextNode(node))
		{
			if (node->Data == data)
				return node;
		}

		return NULL;
	}

	// -------------------------------------------------------------------------------------------
	template <typename T>
	TLISTNODE<T>*	TLIST<T>::GetNode(long index)
	{ // slow
		if (index >= Count) return NULL;

		// TODO: optimise by looking whether or not FirstNode is closer to index then LastNode
		// and use the closest one

		TLISTNODE<T>* node = NULL;
		long i = 0;
		while (node = GetNextNode(node))
		{
			if (i == index) break;
			i++;
		}

		return node;
	}





};






















