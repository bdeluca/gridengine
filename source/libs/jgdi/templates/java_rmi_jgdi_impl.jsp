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
/**
 *  Generated from java_rmi_jgdi_impl.jsp
 *  !!! DO NOT EDIT THIS FILE !!!
 */
<%
  final com.sun.grid.cull.JavaHelper jh = (com.sun.grid.cull.JavaHelper)params.get("javaHelper");
  final com.sun.grid.cull.CullDefinition cullDef = (com.sun.grid.cull.CullDefinition)params.get("cullDef");
  
  
  class JGDIRemoteGenerator extends com.sun.grid.cull.AbstractGDIGenerator {
      
    public JGDIRemoteGenerator(com.sun.grid.cull.CullObject cullObject) {
        super(cullObject.getIdlName(),  jh.getClassName(cullObject), cullObject);
        addPrimaryKeys(cullObject, jh);
    }
    
    public void genImport() {
        if(!(cullObject.getType() == cullObject.TYPE_PRIMITIVE ||
             cullObject.getType() == cullObject.TYPE_MAPPED)) {
%>import com.sun.grid.jgdi.configuration.<%=classname%>;
<%        
        }
    }
    
    public void genGetMethod() {
       
%> 
   /**
    *   Get the <code><%=name%></code> object;
    *   @return the <code><%=name%></code> object
    *   @throws RemoteException on any error
    */
   public <%=classname%> get<%=name%>() throws RemoteException {
      logger.entering("JGDIRemoteImpl","get<%=name%>()");
      try {
        logger.exiting("JGDIRemoteImpl","get<%=name%>()");
        return jgdi.get<%=name%>();
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl","get<%=name%>()",e);
         throw new RemoteException(e.getMessage(), e);
      }
   }  
<%
   } // end of genGetMethod
    
   public void genGetByPrimaryKeyMethod() {
%>       
   /**
    *   Get the <code><%=name%></code> object;
    *   @return the <code><%=name%></code> object
    *   @throws RemoteException on any error
    */
   public <%=classname%> get<%=name%>(<%
      boolean first = true;
      for (java.util.Map.Entry<String, String> entry: primaryKeys.entrySet()) {
         String pkName = entry.getKey();
         String pkType = entry.getValue();
         if(first) {
            first = false;
         } else {
             %>, <% 
         }
         %> <%=pkType%> <%=pkName%><%
      }
   %>) throws RemoteException {
      logger.entering("JGDIRemoteImpl","get<%=name%>()");
      try {
        logger.exiting("JGDIRemoteImpl","get<%=name%>()");
        return jgdi.get<%=name%>(<%
      first = true;
      for (java.util.Map.Entry<String, String> entry: primaryKeys.entrySet()) {
         String pkName = entry.getKey();
         if(first) {
            first = false;
         } else {
             %>, <% 
         }
         %><%=pkName%><%
      }
   %>);
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl","get<%=name%>()",e);
         throw new RemoteException(e.getMessage(), e);
      }
   }  
<%       
   } // genGetByPrimaryKeyMethod
  
    
   public void genGetListMethod() {
%>       
   /**
    *  Get all <code><%=name%></code> objects.
    *  @return a @{link java.util.List} of <code><%=name%></code> objects
    *  @throws RemoteException on any error
    */
   public List get<%=name%>List() throws RemoteException {
      logger.entering("JGDIRemoteImpl","get<%=name%>List()");
      try {
        logger.exiting("JGDIRemoteImpl","get<%=name%>List()");
        return jgdi.get<%=name%>List();
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl","get<%=name%>List()",e);
         throw new RemoteException(e.getMessage(), e);
      }
   }
<%   
   } // end of genGetListMethod
   
   public void genUpdateMethod() {
%>
   /**
    *  Update <%=primaryKeys.size() == 0 ? "a" : "the"%> <code><%=name%></code> object.
    *
    *  @param  obj  the <code><%=name%></code> object with the new values
    *  @throws RemoteException on any error
    */
   public void update<%=name%>(<%=classname%> obj) throws RemoteException {
      logger.entering("JGDIRemoteImpl","update<%=name%>List()");
      try {
        jgdi.update<%=name%>(obj);
        logger.exiting("JGDIRemoteImpl","update<%=name%>List()");
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl","update<%=name%>List()",e);
         throw new RemoteException(e.getMessage(), e);
      }
   }      
<%
   } // end of genUpdateMethod
   
    public void genAddMethod() {
%>
   /**
    *  Add a new <code><%=name%></code> object.
    *
    *  @param obj  the new <code><%=name%></code> object
    *  @throws RemoteException on any error
    */
   public void add<%=name%>(<%=classname%> obj) throws RemoteException {
      logger.entering("JGDIRemoteImpl","add<%=name%>List()");
      try {
        jgdi.add<%=name%>(obj);
        logger.exiting("JGDIRemoteImpl","add<%=name%>List()");
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl","add<%=name%>List()",e);
         throw new RemoteException(e.getMessage(), e);
      }
   }
<%   
   } // end of getAddMethod
   
   public void genDeleteMethod() {
%>
   /**
    *   Delete a <%=name%> object.
    *
    *   @param obj  the <%=name%> with the primary information
    *   @throws RemoteException on any error
    */
   public void delete<%=name%>(<%=classname%> obj) throws RemoteException {
      logger.entering("JGDIRemoteImpl","delete<%=name%>List()");
      try {
        jgdi.delete<%=name%>(obj);
        logger.exiting("JGDIRemoteImpl","delete<%=name%>List()");
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl","delete<%=name%>List()",e);
         throw new RemoteException(e.getMessage(), e);
      }
   }
<%   
   } // end of genDeleteMethod
   
   public void genDeleteByPrimaryKeyMethod() {
%>       
   /**
    *   Delete the <code><%=name%></code> object by primary key
    *   @throws RemoteException on any error
    */
   public void delete<%=name%>(<%
      boolean first = true;
      for (java.util.Map.Entry<String, String> entry: primaryKeys.entrySet()) {
         String pkName = entry.getKey();
         String pkType = entry.getValue();
         if(first) {
            first = false;
         } else {
             %>, <% 
         }
         %> <%=pkType%> <%=pkName%><%
      }
   %>) throws RemoteException {
      logger.entering("JGDIRemoteImpl", "delete<%=name%>()");
      try {
        logger.exiting("JGDIRemoteImpl", "delete<%=name%>()");
        jgdi.delete<%=name%>(<%
      first = true;
      for (java.util.Map.Entry<String, String> entry: primaryKeys.entrySet()) {
         String pkName = entry.getKey();
         if(first) {
            first = false;
         } else {
             %>, <% 
         }
         %><%=pkName%><%
      }
   %>);
      } catch( Exception e ) {
         logger.throwing("JGDIRemoteImpl", "delete<%=name%>()", e);
         throw new RemoteException(e.getMessage(), e);
      }
   }  
<%       
   } // genDeleteByPrimaryKeyMethod
  
  } // end of class JGDIRemoteGenerator
  
  // ---------------------------------------------------------------------------
  // Build the JGDIRemoteGenerator instances
  // ---------------------------------------------------------------------------
  
  java.util.List<JGDIRemoteGenerator> generators = new java.util.ArrayList<JGDIRemoteGenerator>();
  com.sun.grid.cull.CullObject cullObj = null;
  for (String name : cullDef.getObjectNames()) {  
      cullObj = cullDef.getCullObject(name); 
      generators.add(new JGDIRemoteGenerator(cullObj));
   } // end of for
%>
package com.sun.grid.jgdi.rmi;


import com.sun.grid.jgdi.JGDIException;
import java.rmi.RemoteException;
import java.util.List;
import java.util.ArrayList;
import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIFactory;
import java.rmi.server.UnicastRemoteObject;
import java.util.logging.*;

<%  // Import all cull object names;
    for (JGDIRemoteGenerator gen : generators) {
        gen.genImport();
    } // end of while 
%>
/**
 *   <code>JGDIRemoteImpl</code> implements an RMI service for
 *   the {@link com.sun.grid.jgdi.JGDI} interface.
 * 
 */
public class JGDIRemoteImpl extends JGDIRemoteBaseImpl implements JGDIRemote {
   
   /**
    *   Create a new instance of <code>JGDIRemoteImpl</code>.
    *
    *   @param url JGDI connection url
    */
   public JGDIRemoteImpl(String url) 
         throws RemoteException, JGDIException {
      super(url);
   }
   
<%
    for (JGDIRemoteGenerator gen : generators) {
       gen.genMethods();
    } // end of for
%>
}
