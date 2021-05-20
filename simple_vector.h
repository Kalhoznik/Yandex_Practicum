#pragma once

#include "array_ptr.h"
#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>

class ReserveProxyObj{
public:
  explicit ReserveProxyObj(size_t capacty) noexcept
      :capacity_(capacty)
  {
  }
  size_t GetCapacity() const{
    return capacity_;
  }

private:
  size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
  return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
  using Iterator = Type*;
  using ConstIterator = const Type*;

  SimpleVector() noexcept = default;

  // Создаёт вектор из size элементов, инициализированных значением value
  SimpleVector(size_t size, const Type& value = Type())
      :items_(size)
      ,capacity_(size)
      ,size_(size)
  {
    std::fill(begin(),end(),value);
  }

  SimpleVector(const SimpleVector& other)
      :items_(other.capacity_)
      ,capacity_(other.capacity_)
      ,size_(other.size_)
  {
    std::copy(other.begin(),other.end(),begin());
  }

  SimpleVector& operator=(const SimpleVector& rhs) {
    if(this != &rhs){
      auto rhs_copy(rhs);
      swap(rhs_copy);
    }
    return *this;
  }

  SimpleVector( SimpleVector&& other)
      :items_(other.items_.Release())
      ,capacity_(std::exchange(other.capacity_, 0))
      ,size_(std::exchange(other.size_, 0))
  {
  }

  SimpleVector& operator=(SimpleVector&& rhs){
    if(this != &rhs){
      items_ = std::move(rhs.items_);
      size_ = std::exchange(rhs.size_,0);
      capacity_ = std::exchange(rhs.capacity_,0);
    }
    return *this;
  }
  SimpleVector(ReserveProxyObj proxy)
      :items_(proxy.GetCapacity())
        ,capacity_(proxy.GetCapacity())
        ,size_(0)
  {
  }

  void swap(SimpleVector& other) noexcept {
    items_.swap(other.items_);
    std::swap(size_,other.size_);
    std::swap(capacity_,other.capacity_);
  }

  // Создаёт вектор из std::initializer_list
  SimpleVector(std::initializer_list<Type> init)
      :items_(init.size())
      ,capacity_(init.size())
      ,size_(init.size())
  {
    std::move(init.begin(),init.end(),begin());
  }

  void Reserve(size_t new_capacity){
    if(new_capacity > capacity_){
      ArrayPtr<Type>new_items(new_capacity);
      std::move(begin(),end(),new_items.Get());
      items_.swap(new_items);
      capacity_ = new_capacity;
    }
  }

  template<typename Arg>
  void PushBack(Arg&& item) {
    if(capacity_ == 0){
      const  size_t new_capacity = 1;   // выделить в отдельный метод что то типа increase_capacity_initialization
      ArrayPtr<Type>new_items(new_capacity);
      new_items[size_] = std::forward<Arg>(item);
      items_.swap(new_items);
      ++size_;
      capacity_ = new_capacity;
      return;
    }
    if(size_ < capacity_){
      items_[size_] = std::move(item);
      ++size_;
    }else{
      const  size_t new_capacity = capacity_ * 2;   // выделить в отдельный метод что то типа increase_capacity_initialization
      ArrayPtr<Type>new_items(new_capacity);
      std::move(begin(),end(),new_items.Get());
      new_items[size_] = std::forward<Arg>(item);
      items_.swap(new_items);
      ++size_;
      capacity_ = new_capacity;
    }
  }

  // Вставляет значение value в позицию pos.
  // Возвращает итератор на вставленное значение
  // Если перед вставкой значения вектор был заполнен полностью,
  // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1

   template<typename Arg>
  Iterator Insert(ConstIterator pos, Arg value) {

    if(capacity_ == 0 && pos ==  begin()){
      PushBack(std::forward<Arg>(value));
      return begin();
    }

    const auto offset = pos - begin();
    Iterator position = begin() + offset;

    if(size_<capacity_){
      std::move_backward(position,end(),end() + 1 );
      items_[offset] = std::forward<Arg>(value);
    }else{
      const size_t new_capacity = capacity_ * 2;
      ArrayPtr<Type> new_items(new_capacity);
      std::move(begin(),position,new_items.Get());
      new_items[offset] = std::forward<Arg>(value);
      std::move(position,end(),new_items.Get() + offset+ 1);
      items_.swap(new_items);
      capacity_ = new_capacity;
    }
    ++size_;
    return  begin() + offset;
  }

  void PopBack() noexcept {
    assert(size_ != 0);
    --size_;
  }

  // Удаляет элемент вектора в указанной позиции
  Iterator Erase(ConstIterator pos) {
    assert(pos != end());
    const auto offset = pos - begin();
    Iterator position = begin() + offset;
    std::move(position +1, end(), position);
    --size_;
    return begin() + offset;
  }

  // Возвращает количество элементов в массиве
  size_t GetSize() const noexcept {
    // Напишите тело самостоятельно
    return size_;
  }

  // Возвращает вместимость массива
  size_t GetCapacity() const noexcept {
    // Напишите тело самостоятельно
    return capacity_;
  }

  // Сообщает, пустой ли массив
  bool IsEmpty() const noexcept {
    return size_ == 0;
  }

  // Возвращает ссылку на элемент с индексом index
  Type& operator[](size_t index) noexcept {
    assert(index < size_);
    return items_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  const Type& operator[](size_t index) const noexcept {
    assert(index < size_);
    return items_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  Type& At(size_t index) {
    if(index>=size_)
      throw std::out_of_range("index cannot be larger than the size of the array");
    return items_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  const Type& At(size_t index) const {
    if(index>=size_)
      throw std::out_of_range("index cannot be larger than the size of the array");
    return items_[index];
  }

  // Обнуляет размер массива, не изменяя его вместимость
  void Clear() noexcept {
    size_ = 0;
  }

  // Изменяет размер массива.
  // При увеличении размера новые элементы получают значение по умолчанию для типа Type
  void Resize(size_t new_size) {
    if(new_size > capacity_){
      ArrayPtr<Type> new_items(new_size);
      std::move(begin(),end(),new_items.Get());
      std::fill(new_items.Get()+size_,new_items.Get()+new_size,Type());
      items_.swap(new_items);
      capacity_ = new_size;
      size_= new_size;
    }else if (new_size>size_){
      std::fill(end(),begin()+new_size,Type());
      size_ = new_size;
    }else{
      size_ = new_size;
    }
  }

  // Возвращает итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator begin() noexcept {
    return items_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator end() noexcept {
    return items_.Get() +size_;
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator begin() const noexcept {
    return items_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator end() const noexcept {
    return items_.Get() +size_;
  }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cbegin() const noexcept {
    return items_.Get();
  }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cend() const noexcept {
    return items_.Get() +size_;
  }
private:
  ArrayPtr<Type> items_;
  size_t capacity_ = 0;
  size_t size_ = 0;

};
template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return std::lexicographical_compare(lhs.begin(),lhs.end(),rhs.begin(),rhs.end());
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return rhs < lhs;
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return !(rhs<lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return rhs<=lhs;
}

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return std::equal(lhs.begin(),lhs.end(),rhs.begin(),rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return !(lhs == rhs);
}
