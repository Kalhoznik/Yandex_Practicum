#pragma once

#include "array_ptr.h"
#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>

class ReserveProxyObj{
public:
  ReserveProxyObj(size_t capacty)
      :capacity_(capacty)
  {
  }
  size_t GetCapacity(){
    return capacity_;
  }

private:
      size_t capacity_ = 0;
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        :size_(size)
        ,capacity_(size)
        ,items_(size)
    {
      std::fill(begin(),end(),Type());

    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        :size_(size)
        ,capacity_(size)
        ,items_(size)
    {
      std::fill(begin(),end(),value);
    }


    SimpleVector(const SimpleVector& other) {
      CopyAndSwapInitialization(other);
    }

    SimpleVector( SimpleVector&& other){
      capacity_  = std::exchange(other.capacity_, 0);
      size_ = std::exchange(other.size_, 0);
      items_.swap(other.items_);
    }

    SimpleVector(ReserveProxyObj proxy)
        :capacity_(proxy.GetCapacity())
        ,items_(proxy.GetCapacity())
    {}

    void swap(SimpleVector& other) noexcept {
      items_.swap(other.items_);
      std::swap(size_,other.size_);
      std::swap(capacity_,other.capacity_);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
      InitializeAndSwap(rhs);
      return *this;
    }
    SimpleVector& operator=(SimpleVector&& rhs){
      InitializeAndSwap(rhs);
      return *this;
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        :items_(init.size())
    {
      if(init.size()!=0){
        std::copy(std::make_move_iterator(init.begin()),std::make_move_iterator(init.end()),items_.Get());
      }
      size_ =std::move(init.size());
      capacity_= std::move(init.size());
    }

    void Reserve(size_t new_capacity){
      if(new_capacity > capacity_){
        ArrayPtr<Type>new_items(new_capacity);
        std::copy(std::make_move_iterator(begin()),std::make_move_iterator(end()),new_items.Get());
        items_.swap(new_items);
        capacity_ = new_capacity;
      }
    }


    void PushBack(Type item) {
      if(capacity_ == 0){
        size_t new_capacity = 1;   // выделить в отдельный метод что то типа increase_capacity_initialization
        ArrayPtr<Type>new_items(new_capacity);
        new_items[size_] = std::move(item);
        items_.swap(new_items);
        ++size_;
        capacity_ = new_capacity;
        return;
      }

      if(size_<capacity_){
        items_[size_] = std::move(item);
        ++size_;
      }else{
        size_t new_capacity = capacity_ * 2;   // выделить в отдельный метод что то типа increase_capacity_initialization

        ArrayPtr<Type>new_items(new_capacity);
        std::copy(std::make_move_iterator(begin()),std::make_move_iterator(end()),new_items.Get());
        new_items[size_] = std::move(item);
        items_.swap(new_items);
        ++size_;
        capacity_ = new_capacity;
      }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type value) {

      if(capacity_ == 0 && pos ==  begin()){
        PushBack(std::move(value));
        return begin();
      }

      Iterator result = nullptr;
      auto offset = pos - begin();
      Iterator pure_iterator = begin() + offset;

      if(size_<capacity_){

        std::copy_backward(std::make_move_iterator(pure_iterator),std::make_move_iterator(end()),end() + 1 );
        items_[offset] = std::move(value);
        ++size_;
        result = begin() + offset;
      }else{
        size_t new_capacity = capacity_ * 2;   // выделить в отдельный метод что то типа increase_capacity_initialization
        ArrayPtr<Type>new_items(new_capacity);
        std::copy(std::make_move_iterator(begin()),std::make_move_iterator(pure_iterator),new_items.Get());
        new_items[offset] = std::move(value);
        std::copy(std::make_move_iterator(pure_iterator),std::make_move_iterator(end()),new_items.Get() + offset+ 1);
        items_.swap(new_items);
        ++size_;
        capacity_ = new_capacity;
        result = begin() + offset;
      }
      return  result;
    }

   void PopBack() noexcept {
     --size_;
   }

   // Удаляет элемент вектора в указанной позиции
   Iterator Erase(ConstIterator pos) {
     auto offset = pos - begin();
     Iterator pure_iterator = begin() + offset;
     if(pure_iterator + 1 != end()){
       std::copy(std::make_move_iterator(pure_iterator +1), std::make_move_iterator(end()), pure_iterator);
     }
     --size_;
     return pure_iterator;
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
      return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
       return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
      if(index>=size_)
        throw std::out_of_range("out_of_range");
      return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
      if(index>=size_)
        throw std::out_of_range("out_of_range");
      return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
      size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
      if(new_size >= capacity_){
        ArrayPtr<Type> new_items(new_size);

        std::copy(begin(),end(),new_items.Get());
        std::fill(new_items.Get()+size_,new_items.Get()+new_size,Type());

        items_.swap(new_items);

        capacity_ = std::max(new_size,capacity_*2);
        size_= new_size;
      }else if ((new_size<capacity_)&&(new_size>=size_)){
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
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> items_;

    template<typename Container> //добавить проверку на пустоту вектора
    void CopyAndSwapInitialization(const Container& other_container){
      SimpleVector<Type> tmp(other_container.size_);
      std::copy(other_container.begin(),other_container.end(),tmp.begin());
      swap(tmp);
    }

    void InitializeAndSwap(SimpleVector&& vector){
      if(this != &vector){
        auto rhs_copy(vector);
        swap(rhs_copy);
      }
    }

    void InitializeAndSwap(const SimpleVector& vector){
      if(this != &vector){
        auto rhs_copy(vector);
        swap(rhs_copy);
      }
    }

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
  // Заглушка. Напишите тело самостоятельно
  return !(rhs<lhs);;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  // Заглушка. Напишите тело самостоятельно
  return rhs<=lhs;
}
template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

  return !(lhs<rhs || lhs >rhs);;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
  // Заглушка. Напишите тело самостоятельно
  return !(lhs == rhs);
}

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
  return ReserveProxyObj(capacity_to_reserve);
}
