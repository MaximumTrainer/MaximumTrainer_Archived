/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"

#include <vector>
#include <functional>

//#include "usb_device_handle_si.hpp"
//#include "usb_device_handle_libusb.hpp"


//#include <list>

//using namespace std;

/*
struct SI
{
   static const USHORT ANT_VID = 0x0FCF;
   static const USHORT ANT_PID = 0x1004;

   //defines used by AutoInit
   static const UCHAR ANT_PRODUCT_DESCRIPTION[255];
   static const USHORT DEFAULT_BAUD_RATE = 50000;
};
const UCHAR SI::ANT_PRODUCT_DESCRIPTION[255] = "Dynastream USB Interface Board";  //"ANT USB Stick"


auto_ptr<const ANTDeviceList> ANTDeviceList::GetDeviceList(ULONG ulDeviceTypeField_)
{
   return auto_ptr<const ANTDeviceList>(new ANTDeviceList(ulDeviceTypeField_));
}

/*
void USBDeviceList::FreeDeviceList(const USBDeviceList*& pclDeviceList_)
{
   delete pclDeviceList_;
   pclDeviceList_ = NULL;
   return;
}
*//*

ANTDeviceList::ANTDeviceList(ULONG ulDeviceTypeField_)
: USBDeviceList()
{
   if( (ulDeviceTypeField_ & DeviceType::SI_LABS) != 0)
   {
      const list<const USBDeviceSI*> clDeviceSIList = USBDeviceHandleSI::GetDeviceList();  //!!There is a list copy here!
      //clDeviceList.insert(clDeviceList.end(), clDeviceSIList.begin(), clDeviceSIList.end());  //!!do we have to make an iterator?
      Add(clDeviceSIList.begin(), clDeviceSIList.end());
   }

   if( (ulDeviceTypeField_ & DeviceType::LIBUSB) != 0)
   {
      const list<const USBDeviceLibusb*> clDeviceLibusbList = USBDeviceHandleLibusb::GetDeviceList();
      //clDeviceList.insert(clDeviceList.end(), clDeviceLibusbList.begin(), clDeviceLibusbList.end());  //!!do we have to make an iterator?
      Add(clDeviceLibusbList.begin(), clDeviceLibusbList.end());
   }

   //ulSize = clDeviceList.size();
   return;
}

ANTDeviceList::~ANTDeviceList()
{
      /*
   list<const USBDevice*>::iterator i;
   for(i = clDeviceList.begin(); i != clDeviceList.end(); i++)
   {
      const USBDevice*& pclDevice = *i;
      if(pclDevice != NULL)
      {
         delete pclDevice;  //!!Make sure the device specific lists do not delete what they give to us
         pclDevice = NULL;
      }
   }

   clDeviceList.clear();
   ulSize = 0;
   *//*
   return;
}
*/

#define USBTemplateDeviceList    USBDeviceList<USBDeviceType>

/*
template <typename USBDeviceType>
template<typename Container_>
USBTemplateDeviceList::USBDeviceList(const Container_& tContainer_)
{
   Add(tContainer_.begin(), tContainer_.end());
   return;
}
*/

template <typename USBDeviceType>
USBTemplateDeviceList::~USBDeviceList()
{
   Clear();
   return;
}


template <typename USBDeviceType>
template <typename InputIterator>
USBTemplateDeviceList::USBDeviceList(InputIterator tFirst_, InputIterator tLast_)
{
   Add(tFirst_, tLast_);
   return;
}

template <typename USBDeviceType>
USBTemplateDeviceList::USBDeviceList(const USBTemplateDeviceList& clDeviceList_)
{
   Add(clDeviceList_);
   return;
}

template <typename USBDeviceType>
template<typename USBDeviceType_>
USBTemplateDeviceList::USBDeviceList(const USBDeviceList<USBDeviceType_>& clDeviceList_)
{
   Add(clDeviceList_);
   return;
}



template <typename USBDeviceType>
USBTemplateDeviceList& USBTemplateDeviceList::operator=(const USBTemplateDeviceList& clDeviceList_)
{
   if(this == &clDeviceList_)
      return *this;

   Clear();
   Add(clDeviceList_);
   return *this;
}


template <typename USBDeviceType>
template<typename USBDeviceType_>
USBTemplateDeviceList& USBTemplateDeviceList::operator=(const USBDeviceList<USBDeviceType_>& clDeviceList_)
{
   if(this == &clDeviceList_)
      return *this;

   Clear();
   Add(clDeviceList_);
   return *this;
}



template <typename USBDeviceType>
template <typename InputIterator>
void USBTemplateDeviceList::Add(InputIterator tFirst_, InputIterator tLast_)
{
   for(InputIterator i=tFirst_; i != tLast_; i++)
      Add(*i);

   //insert(clDeviceList.end(), tFirst_, tLast_);
   return;
}


template <typename USBDeviceType>
template <typename USBDeviceType_>
void USBTemplateDeviceList::Add(const USBDeviceList<USBDeviceType_>& clDeviceList_)
{
   ULONG ulSize = clDeviceList_.GetSize();
   for(ULONG i=0; i<ulSize; i++)
      Add(clDeviceList_[i]);  //!!keep track of iterator ourselves?

   //insert(clDeviceList.end(), clDeviceList_.clDeviceList.begin(), clDeviceList_.clDeviceList.end());    //!!do we have to make an iterator?
   return;
}


template <typename USBDeviceType>
void USBTemplateDeviceList::Add(const USBDeviceType& tDevice_)  //!!Return the address of the element?  Then we don't need GetAddress().
{
   //Container::const_iterator clIterator;
   //advance(clIterator, ulSize);

   clDeviceList.insert(clDeviceList.end(), new USBDeviceType(tDevice_));
   //clDeviceList.push_back(tDevice_);  //!! container universal?
   return;
}

template <typename USBDeviceType>
void USBTemplateDeviceList::Clear()
{
   size_t uiSize = clDeviceList.size();
   for(size_t i=0; i<uiSize; i++)
      delete clDeviceList[i];

   clDeviceList.clear();

   return;
}



template <typename USBDeviceType>
USBTemplateDeviceList USBTemplateDeviceList::GetSubList(typename USBDeviceList::CompareFunc pfCompareFunc_) const  //!!Copy!
{
   USBTemplateDeviceList clSubList;

   if(pfCompareFunc_ == NULL)
      return clSubList;

   typename Container::const_iterator clIterator;
   for(clIterator = clDeviceList.begin(); clIterator != clDeviceList.end(); clIterator++)
   {
      USBDeviceType const& tDevice = **clIterator;
      if(pfCompareFunc_(tDevice) == TRUE)
         clSubList.Add(tDevice);
   }

   return clSubList;
}



template <typename USBDeviceType>
const USBDeviceType& USBTemplateDeviceList::operator[](ULONG ulNum_) const
{
   if(ulNum_ >= clDeviceList.size())
      throw 0; //!!

   typename Container::const_iterator clIterator;
   clIterator = clDeviceList.begin();
   std::advance(clIterator, ulNum_);
   return **clIterator;
}


template <typename USBDeviceType>
const USBDeviceType* USBTemplateDeviceList::GetAddress(ULONG ulNum_) const
{
   if(ulNum_ >= clDeviceList.size())
      throw 0; //!!

   typename Container::const_iterator clIterator;
   clIterator = clDeviceList.begin();
   std::advance(clIterator, ulNum_);
   return *clIterator;
}
