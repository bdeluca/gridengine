/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
// Codine_Complex_impl.cpp
// implementation for Complex object

#include <pthread.h>

#include "Complex_impl.h"
#include "Master_impl.h"
#include "qidl_common.h"

extern "C" {
#include "cod_api.h"
#include "cod_answerL.h"
#include "cod_complexL.h"
#include "cod_m_event.h"
#include "codrmon.h"
#include "cod_log.h"
#include "qmaster.h"
}

#ifdef HAVE_STD
using namespace std;
#endif

extern lList* Master_Complex_List;

Codine_Complex_impl::Codine_Complex_impl(const char* _name, CORBA_ORB_var o)
   : Codine_Complex_implementation(_name, o) {
   QENTER("Codine_Complex_impl::Codine_Complex_impl");
   DPRINTF(("Name: %s\n", _name));
}

Codine_Complex_impl::Codine_Complex_impl(const char* _name, const time_t& tm, CORBA_ORB_var o)
   : Codine_Complex_implementation(_name, tm, o) {
   QENTER("Codine_Complex_impl::Codine_Complex_impl(time)");
   DPRINTF(("Name: %s\n", _name));
   
   AUTO_LOCK_MASTER;

   self = lCreateElem(CX_Type);
   lSetString(self, CX_name, (char*)_name);
}
   
Codine_Complex_impl::~Codine_Complex_impl() {
   QENTER("Codine_Complex_impl::~Codine_Complex_impl");
   DPRINTF(("Name: %s\n", (const char*)key));

   if(creation != 0)
      lFreeElem(&self);
}

// inherited from Codine_Object
void Codine_Complex_impl::destroy(CORBA_Context* ctx) {
   QENTER("Codine_Complex_impl::destroy");
   
   AUTO_LOCK_MASTER;

   qidl_authenticate(ctx);

   getSelf();

   // CORBA object only ?
   if(creation != 0) {
      orb->disconnect(this);
      // The CORBA object itself will be destroyed by
      // the master, some time later
      return;
   }

   // make api request
   lListPtr      lp;
   lListPtr      alp;

   lp = lCreateList("My Complex List", CX_Type);
   lAppendElem(lp, lCopyElem(self));

   alp = cod_api(COD_COMPLEX_LIST, COD_API_DEL, &lp, NULL, NULL);
   throwErrors(alp);
}

lListElem* Codine_Complex_impl::getSelf() {
   QENTER("Codine_Complex_impl::getSelf");
   
   if(creation != 0)
      return self;

   // if newSelf is set, then use newSelf, because newSelf is
   // valid (if !NULL) and definitely newer than self
   if(newSelf)
      return self = newSelf;

   AUTO_LOCK_MASTER;

   lCondition* cp = lWhere("%T(%I==%s)", CX_Type, CX_name, (const char*)key);
   self = lFindFirst(Master_Complex_List, cp);
   lFreeWhere(&cp);
    
   if(!self) {  
      // we must not destroy ourselves here because the other thread
      // might also have done so already. if the object is still
      // alive at this point and will NOT be destroyed automatically
      // (by OB runtime) after this exception, then there is some logical
      // error in the code: The codine kernel did not notify the qidl
      // layer of the death of the object
      throw Codine_ObjDestroyed();
   }

   return self;
}


void Codine_Complex_impl::add(CORBA_Context* ctx) {
   QENTER("Codine_Complex_impl::add");

   // this Complex has been added already ?
   if(creation == 0)
      return;
   
   AUTO_LOCK_MASTER;

   qidl_authenticate(ctx);

   // make api request
   lListPtr      lp;
   lListPtr      alp;
   lEnumeration* what;
   
   lp = lCreateList("My Complex List", CX_Type);
   lAppendElem(lp, self);

   what = lWhat("%T(ALL)", CX_Type);

   alp = cod_api(COD_COMPLEX_LIST, COD_API_ADD, &lp, NULL, what);
   lFreeWhat(what);

   throwErrors(alp);

   creation = 0;
}

Codine_cod_string Codine_Complex_impl::get_name(CORBA_Context* ctx) {
   QENTER("Codine_Complex_impl::get_name");
   
   AUTO_LOCK_MASTER;

   qidl_authenticate(ctx);

   getSelf();

   Codine_cod_string foo = CORBA_string_dup((creation!=0)?lGetString(self, CX_name):(const char*)key);
   return foo;
}

void Codine_Complex_impl::changed(lListElem* _newSelf) {
   QENTER("Codine_Complex_impl::changed");

   AUTO_LOCK_MASTER;

   // set self variable for now
   // setting newself forces getSelf() NOT to query for myself in
   // the master list
   self = newSelf = _newSelf;

   // store event counter locally, to pass it back to the client later
   // This works as long as there is only one CORBA thread, so there
   // can be only one request dispatched at one time.
   // If more client requests were able to execute simultanously,
   // they would overwrite this local variable => !&%$&/�$�%
   lastEvent = qidl_event_count;

   // build header
   Codine_event ev;
   ev.type = Codine_ev_mod;
   ev.obj  = COD_COMPLEX_LIST;
   ev.name = CORBA_string_dup((creation!=0)?lGetString(newSelf, CX_name):(const char*)key);
   ev.id   = 0; // not needed for complex
   ev.ref  = Codine_Complex_impl::_duplicate(this);
   ev.count= qidl_event_count;

   // get state
   Codine_contentSeq* cs = get_content(Codine_Master_impl::instance()->getMasterContext());
   ev.changes = *cs;
   delete cs;

   CORBA_Any any;
   any <<= ev;
   Codine_Master_impl::instance()->addEvent(any);

   // now we're ourselves again :-(
   newSelf = 0;
}
