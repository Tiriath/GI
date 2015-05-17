/// \file gilib.h
/// \brief Base classes and methods
///
/// \author Raffaele D. Facendola

#pragma once

#include <string>
#include <locale>
#include <codecvt>

#include "macros.h"

namespace gi_lib{

	class WeakObject;

	template <typename TObject>
	class ObjectPtr;

	/// \brief Don't care.
	/// Use this structure when you don't need a parameter in a lambda expression
	struct _{

		template <typename... TArguments>
		_(TArguments&&...){}

	};
	
	/// \brief Base interface for every object whose life cycle is determined by a reference counter.
	/// \author Raffaele D. Facendola
	class Object{

	public:

		/// \brief Default destructor.
		/// The object is initialized with 0 strong reference count and no weak reference object.
		Object();

		/// \brief Virtual destructor.
		virtual ~Object();

		/// \brief No assignment operator.
		Object& operator=(const Object&) = delete;

		/// \brief Add a strong reference to the object.
		void AddRef();

		/// \brief Release a strong reference to the object.
		/// If the reference count drops to 0, this instance will be destroyed immediately.
		void Release();
		
	private:

		size_t ref_count_;				///< \brief Number of strong references to this object.

		WeakObject* weak_object_;		///< \brief Pointer to a weak object helper (the pointer point to this object).

	};

	/// \brief Represents a weak reference to an object.
	/// A weak reference won't prevent an object from being destroyed but can be locked to get a strong reference to it.
	/// \author Raffaele D. Facendola
	class WeakObject{

	public:

		/// \brief Create a weak reference to the given object.
		/// \param subject Object to point.
		WeakObject(Object* subject);

		/// \brief Release the subject.
		/// This method is called whenever the subject is destroyed.
		void Clear();

		/// \brief Add a weak reference to the object.
		void AddRef();

		/// \brief Release a weak reference to the object.
		/// If the reference count drops to 0, this instance will be destroyed immediately.
		void Release();

	private:

		size_t weak_count_;				///< \brief Number of weak references to the object.

		Object* subject_;				///< \brief Pointed object

	};

	/// \brief Strong reference to an object.
	/// The pointer will add a reference during initialization and remove one during destruction.
	/// \remarks This class is not thread safe.
	/// \todo Make this class thread safe.
	template <typename TObject>
	class ObjectPtr{

	public:

		/// \brief Create an empty pointer.
		ObjectPtr();

		/// \brief Defines a pointer to an object.
		/// \param object Object that will be pointed by this pointer.
		ObjectPtr(TObject* object);

		/// \brief Copy constructor.
		/// \param other Other pointer to copy.
		ObjectPtr(const ObjectPtr<TObject>& other);

		/// \brief Move constructor.
		/// \param other Instance to move.
		ObjectPtr(ObjectPtr<TObject>&& other);

		/// \brief Destructor.
		/// Decreases by one the reference count of the pointed object, if any.
		~ObjectPtr();

		/// \brief Unified assignment operator.
		ObjectPtr<TObject>& operator=(ObjectPtr<TObject> other);

		/// \brief Equality operator.
		/// \return Returns true if both this instance and the specified one points to the same object, returns false otherwise.
		bool operator==(const ObjectPtr<TObject>& other) const;

		/// \brief Inequality operator.
		/// \return Returns true if this instance and the specified one points to different objects, returns false otherwise.
		bool operator!=(const ObjectPtr<TObject>& other) const;

		/// \brief Used to validate the pointed object.
		/// \return Returns true if the pointed object is not null, returns false otherwise.
		operator bool() const;

		/// \brief Arrow operator.
		/// Access the managed object.
		TObject* operator->();

		/// \brief Arrow operator.
		/// Access the managed object.
		const TObject* operator->() const;

		/// \brief Release the pointed object.
		void Release();

	private:

		/// \brief Add a reference to the pointed object.
		void AddRef();

		/// \brief Swaps this instance with another one.
		void Swap(ObjectPtr<TObject>& other);

		TObject* object_ptr_;			/// \brief Pointer to the object.

	};


	/// \brief Converts a string to a wstring.
	inline std::wstring to_wstring(const std::string& string)
	{

		return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(string);

	}

	/// \brief Converts a wstring to a string.
	inline std::string to_string(const std::wstring& wstring)
	{

		return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(wstring);

	}

	///////////////////////////////// OBJECT //////////////////////////////////

	inline Object::Object() :
		ref_count_(0),
		weak_object_(nullptr){}

	inline Object::~Object(){

		weak_object_->Clear();	// Clear the subject.

	}

	inline void Object::AddRef(){

		++ref_count_;

	}

	inline void Object::Release(){

		--ref_count_;

		if (ref_count_ == 0){

			delete this;

		}

	}

	///////////////////////////////// WEAK OBJECT /////////////////////////////////

	inline WeakObject::WeakObject(Object* subject) :
		subject_(subject),
		weak_count_(0){}

	inline void WeakObject::Clear(){

		subject_ = nullptr;

	}

	inline void WeakObject::AddRef(){

		++weak_count_;

	}

	inline void WeakObject::Release(){

		--weak_count_;

		if (weak_count_ == 0){

			delete this;

		}

	}


	///////////////////////////////// OBJECT PTR //////////////////////////////////

	template <typename TObject>
	inline ObjectPtr<TObject>::ObjectPtr() :
		object_ptr_(nullptr){}

	template <typename TObject>
	inline ObjectPtr<TObject>::ObjectPtr(TObject* object) :
		object_ptr_(object){

		AddRef();

	}

	template <typename TObject>
	inline ObjectPtr<TObject>::ObjectPtr(const ObjectPtr<TObject>& other) :
		ObjectPtr(other.object_ptr_){}

	template <typename TObject>
	inline ObjectPtr<TObject>::ObjectPtr(ObjectPtr<TObject>&& other) :
	object_ptr_(other.object_ptr_){

		other.object_ptr_ = nullptr;

	}

	template <typename TObject>
	inline ObjectPtr<TObject>::~ObjectPtr(){

		Release();

	}

	template <typename TObject>
	inline ObjectPtr<TObject>& ObjectPtr<TObject>::operator=(ObjectPtr<TObject> other){

		other.Swap(*this);

	}

	template <typename TObject>
	inline bool ObjectPtr<TObject>::operator==(const ObjectPtr<TObject>& other) const{

		return object_ptr_ == other.object_ptr_;

	}

	template <typename TObject>
	inline bool ObjectPtr<TObject>::operator!=(const ObjectPtr<TObject>& other) const{

		return object_ptr_ != other.object_ptr_;

	}

	template <typename TObject>
	inline ObjectPtr<TObject>::operator bool() const{

		return object_ptr_ != nullptr;

	}

	template <typename TObject>
	inline TObject* ObjectPtr<TObject>::operator->(){

		return object_ptr_;

	}

	template <typename TObject>
	inline const TObject* ObjectPtr<TObject>::operator->() const{

		return object_ptr_;

	}

	template <typename TObject>
	inline void ObjectPtr<TObject>::Swap(ObjectPtr<TObject>& other){

		std::swap(object_ptr_, 
				  other.object_ptr_);

	}

	template <typename TObject>
	inline void ObjectPtr<TObject>::Release(){

		if (object_ptr_){

			object_ptr_->Release();

			object_ptr_ = nullptr;

		}

	}

	template <typename TObject>
	inline void ObjectPtr<TObject>::AddRef(){

		if (object_ptr_){

			object_ptr_->AddRef();

		}

	}

}