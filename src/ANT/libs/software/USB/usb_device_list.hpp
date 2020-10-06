/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef USB_DEVICE_LIST_HPP
#define USB_DEVICE_LIST_HPP

#include "types.h"

#include <vector>
#include <functional>


//template <typename USBDeviceType, typename Container = std::vector<USBDeviceType> >
template <typename USBDeviceType>
class USBDeviceList
{
  public:

   //CONSTRUCTORS//

   USBDeviceList() { return; }
   virtual ~USBDeviceList();

   USBDeviceList(const USBDeviceList& clDeviceList_);

   template<typename USBDeviceType_>
   USBDeviceList(const USBDeviceList<USBDeviceType_>& clDeviceList_);

   template <typename InputIterator>
   USBDeviceList(InputIterator tFirst_, InputIterator tLast_);


   //ASSIGNMENT OPERATOR//

   USBDeviceList& operator=(const USBDeviceList& clDeviceList_);

   template<typename USBDeviceType_>
   USBDeviceList& operator=(const USBDeviceList<USBDeviceType_>& clDeviceList_);


   //MUTATORS//

   void Add(const USBDeviceType& tDevice_);

   template<typename USBDeviceType_>
   void Add(const USBDeviceList<USBDeviceType_>& clDeviceList_);

   template<typename InputIterator>
   void Add(InputIterator tFirst_, InputIterator tLast_);

   void Clear();

   //ACCESSORS//


   typedef BOOL (*CompareFunc)(USBDeviceType const& tDevice_);  //!!const USBDeviceType&
   //typedef std::unary_function<USBDeviceType, BOOL> CompareFunc;  //!! unary_function<const USBDeviceType&,bool>
   USBDeviceList GetSubList(CompareFunc pfCompareFunc_) const;  //!!Use unary_function instead! (scope, no bare pointer)

   const USBDeviceType& operator[](ULONG ulNum_) const;
   ULONG GetSize() const { return (ULONG)clDeviceList.size(); }  //!!keep track of this so we can cache it ourselves?
   const USBDeviceType* GetAddress(ULONG ulNum_) const;  //!!Don't want to have this


  private:

   //typedef std::vector<auto_ptr<USBDeviceType>> Container;  //!!Do not use special container functions like sorting because we are holding auto_ptrs!
   typedef std::vector<USBDeviceType*> Container;  //we hold on to pointers so that we can hold copies and still have polymorphism
                                                   //we also hold on to pointers so that no matter where std::vector recopies stuff, we always have a constant place where our elements are
   Container clDeviceList;
};

#include "usb_device_list_template.hpp"


/*
//!!Could just be a normal class with a refresh function (but then we can't guarantee the list is const)
class ANTDeviceList : public USBDeviceList<const USBDevice*>
{
  public:
   static std::auto_ptr<const ANTDeviceList> GetDeviceList(ULONG ulDeviceTypeField_ = 0xFFFFFFFF);  //this function keeps people from making a non-const instance of this class
   //static void FreeDeviceList(const USBDeviceList*& pclDeviceList_);

  protected:
   ANTDeviceList(ULONG ulDeviceTypeField_);
   virtual ~ANTDeviceList();

  private:


   friend std::auto_ptr<const ANTDeviceList>;  //So that auto_ptr can be the only class to destroy an instance
};
*/

#endif //USB_DEVICE_LIST_HPP