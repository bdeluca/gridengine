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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#ifndef NO_SGE_COMPILE_DEBUG   
#   define NO_SGE_COMPILE_DEBUG
#endif

#include "sge_all_listsL.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_schedd.h"
#include "sge_time.h"
#include "sge_orders.h"
#include "sge_job_schedd.h"
#include "scheduler.h"
#include "sgeee.h"
#include "sge_support.h"
#include "sge_schedd_conf.h"
#include "sge_usageL.h"
#include "sge_userprj.h"
#include "sgeobj/sge_sharetree.h"
#include "valid_queue_user.h"

const long sge_usage_interval = SGE_USAGE_INTERVAL;
static double sge_decay_rate;
static double sge_decay_constant;

/*--------------------------------------------------------------------
 * decay_usage - decay usage for the passed usage list
 *--------------------------------------------------------------------*/

static void
decay_usage( lList *usage_list,
             const lList *decay_list,
             u_long curr_time,
             u_long usage_time_stamp )
{
   lListElem *usage;
   static int ua_value_pos = -1;
   static int ua_name_pos = -1;

   if (ua_value_pos == -1) {
      lListElem *ua_elem = lCreateElem(UA_Type);
      ua_value_pos = lGetPosViaElem(ua_elem, UA_value);
      ua_name_pos = lGetPosViaElem(ua_elem, UA_name);
      lFreeElem(ua_elem);
   }

   if (usage_list) {

      double decay, default_decay;
      if (curr_time > usage_time_stamp) {

         default_decay = pow(sge_decay_constant,
                     (double)(curr_time - usage_time_stamp) /
                     (double)sge_usage_interval);

         for_each(usage, usage_list) {
            lListElem *decay_elem;
            if (decay_list &&
                ((decay_elem = lGetElemStr(decay_list, UA_name,
                      lGetPosString(usage, ua_name_pos)))))
               decay = pow(lGetPosDouble(decay_elem, ua_value_pos),
                     (double)(curr_time - usage_time_stamp) /
                     (double)sge_usage_interval);
            else
               decay = default_decay;
            lSetPosDouble(usage, ua_value_pos,
                          lGetPosDouble(usage, ua_value_pos) * decay);
         }
      }
   }
   return;
}

/*--------------------------------------------------------------------
 * decay_userprj_usage - decay usage for the passed user/project object
 *--------------------------------------------------------------------*/

void
decay_userprj_usage( lListElem *userprj,
                     const lList *decay_list,
                     u_long seqno,
                     u_long curr_time )
{
   u_long usage_time_stamp;
   static int up_usage_seqno_pos = -1;
   static int up_usage_time_stamp_pos = -1;
   static int up_usage_pos = -1;
   static int up_project_pos = -1;
   static int upp_usage_pos = -1;

   if (up_usage_seqno_pos == -1) {
      lListElem *up_elem = lCreateElem(UP_Type);
      lListElem *upp_elem = lCreateElem(UPP_Type);
      up_usage_seqno_pos = lGetPosViaElem(up_elem, UP_usage_seqno);
      up_usage_time_stamp_pos = lGetPosViaElem(up_elem, UP_usage_time_stamp);
      up_usage_pos = lGetPosViaElem(up_elem, UP_usage);
      up_project_pos = lGetPosViaElem(up_elem, UP_project);
      upp_usage_pos = lGetPosViaElem(upp_elem, UPP_usage);
      lFreeElem(up_elem);
      lFreeElem(upp_elem);
   }

   if (userprj && seqno != lGetPosUlong(userprj, up_usage_seqno_pos)) {

   /*-------------------------------------------------------------
    * Note: In order to decay usage once per decay interval, we
    * keep a time stamp in the user/project of when it was last
    * decayed and then apply the approriate decay based on the time
    * stamp. This allows the usage to be decayed on the scheduling
    * interval, even though the decay interval is different than
    * the scheduling interval.
    *-------------------------------------------------------------*/

      usage_time_stamp = lGetPosUlong(userprj, up_usage_time_stamp_pos);

      if (usage_time_stamp > 0) {
         lListElem *upp;

         decay_usage(lGetPosList(userprj, up_usage_pos), decay_list,
                     curr_time, usage_time_stamp);

         for_each(upp, lGetPosList(userprj, up_project_pos))
            decay_usage(lGetPosList(upp, upp_usage_pos), decay_list,
                        curr_time, usage_time_stamp);

      }

      lSetPosUlong(userprj, up_usage_time_stamp_pos, curr_time);
      if (seqno != (u_long) -1)
	 lSetPosUlong(userprj, up_usage_seqno_pos, seqno);

   }

   return;
}


/*--------------------------------------------------------------------
 * calculate_decay_constant - calculates decay rate and constant based
 * on the decay half life and usage interval. The halftime argument
 * is in minutes.
 *--------------------------------------------------------------------*/

void
calculate_decay_constant( double halftime,
                          double *decay_rate,
                          double *decay_constant )
{
   if (halftime < 0) {
      *decay_rate = 1.0;
      *decay_constant = 0;
   } else if (halftime == 0) {
      *decay_rate = 0;
      *decay_constant = 1.0;
   } else {
      *decay_rate = - log(0.5) / (halftime * 60);
      *decay_constant = 1 - (*decay_rate * sge_usage_interval);
   }
   return;
}


/*--------------------------------------------------------------------
 * calculate_default_decay_constant - calculates the default decay
 * rate and constant based on the decay half life and usage interval.
 * The halftime argument is in hours.
 *--------------------------------------------------------------------*/

void
calculate_default_decay_constant( int halftime )
{
   calculate_decay_constant(halftime*60.0, &sge_decay_rate, &sge_decay_constant);
}


/*--------------------------------------------------------------------
 * sge_for_each_node - visit each node and call the supplied function
 * until a non-zero return code is returned.
 *--------------------------------------------------------------------*/

int
sge_for_each_share_tree_node( lListElem *node,
                              sge_node_func_t func,
                              void *ptr )
{
   int retcode=0;
   lList *children;
   lListElem *child_node;
   static int sn_children_pos = -1;

   if (!node)
      return 0;

   if (sn_children_pos == -1)
      sn_children_pos = lGetPosViaElem(node, STN_children);

   if ((retcode = (*func)(node, ptr)))
      return retcode;

   if ((children = lGetPosList(node, sn_children_pos))) {
      for_each(child_node, children) {
         if ((retcode = sge_for_each_share_tree_node(child_node, func, ptr)))
            break;
      }
   }

   return retcode;
}


/*--------------------------------------------------------------------
 * zero_node_fields - zero out the share tree node fields that are 
 * passed to the qmaster from schedd and are displayed at qmon
 *--------------------------------------------------------------------*/

int
sge_zero_node_fields( lListElem *node,
                      void *ptr )
{
   static int sn_m_share_pos = -1;
   static int sn_adjusted_current_proportion_pos = -1;
   static int sn_job_ref_count_pos = -1;

   if (sn_m_share_pos == -1) {
      sn_m_share_pos = lGetPosViaElem(node, STN_m_share);
      sn_adjusted_current_proportion_pos =
            lGetPosViaElem(node, STN_adjusted_current_proportion);
      sn_job_ref_count_pos = lGetPosViaElem(node, STN_job_ref_count);
   }

   lSetPosDouble(node, sn_m_share_pos, 0);
   lSetPosDouble(node, sn_adjusted_current_proportion_pos, 0);
   lSetPosUlong(node, sn_job_ref_count_pos, 0);
   return 0;
}


/*--------------------------------------------------------------------
 * sge_init_node_fields - zero out the share tree node fields that are 
 * passed to the qmaster from schedd and are displayed at qmon
 *--------------------------------------------------------------------*/

int
sge_init_node_fields( lListElem *root )
{
   return sge_for_each_share_tree_node(root, sge_zero_node_fields, NULL);
}


/*--------------------------------------------------------------------
 * sge_calc_node_usage - calculate usage for this share tree node
 * and all descendant nodes.
 *--------------------------------------------------------------------*/

double
sge_calc_node_usage( lListElem *node,
                     const lList *user_list,
                     const lList *project_list,
                     const lList *decay_list,
                     u_long curr_time,
                     const char *projname,
                     u_long seqno )
{
   double usage_value = 0;
   int project_node = 0;
   lListElem *child_node;
   lList *children;
   lListElem *userprj = NULL;
   const lList *usage_weight_list = NULL;
   lList *usage_list=NULL;
   lListElem *usage_weight, *usage_elem;
   double sum_of_usage_weights = 0;
   const char *usage_name;
   static int sn_children_pos = -1;
   static int sn_combined_usage_pos = -1;
   static int sn_name_pos = -1;
   static int sn_usage_list_pos = -1;
   static int ua_name_pos = -1;
   static int ua_value_pos = -1;
   static int up_usage_pos = -1;

   DENTER(TOP_LAYER, "sge_calc_node_usage");

   if (sn_children_pos == -1) {
      lListElem *ua_elem = lCreateElem(UA_Type);
      lListElem *sc_elem = lCreateElem(SC_Type);
      lListElem *up_elem = lCreateElem(UP_Type);
      sn_children_pos = lGetPosViaElem(node, STN_children);
      sn_combined_usage_pos = lGetPosViaElem(node, STN_combined_usage);
      sn_usage_list_pos = lGetPosViaElem(node, STN_usage_list);
      sn_name_pos = lGetPosViaElem(node, STN_name);
      ua_name_pos = lGetPosViaElem(ua_elem, UA_name);
      ua_value_pos = lGetPosViaElem(ua_elem, UA_value);
      up_usage_pos = lGetPosViaElem(up_elem, UP_usage);
      lFreeElem(ua_elem);
      lFreeElem(sc_elem);
      lFreeElem(up_elem);
   }

   children = lGetPosList(node, sn_children_pos);
   if (!children) {

      if (projname) {

         /*-------------------------------------------------------------
          * Get usage from project usage sub-list in user object
          *-------------------------------------------------------------*/

         if ((userprj = userprj_list_locate(user_list, 
                                      lGetPosString(node, sn_name_pos)))) {

            lList *projects = lGetList(userprj, UP_project);
            lListElem *upp;

            if (projects)
               if ((upp=lGetElemStr(projects, UPP_name, projname)))
                  usage_list = lGetList(upp, UPP_usage);

         }

      } else {

         /*-------------------------------------------------------------
          * Get usage directly from corresponding user or project object
          *-------------------------------------------------------------*/

         if ((userprj = userprj_list_locate(user_list, 
                                      lGetPosString(node, sn_name_pos)))) {

            usage_list = lGetList(userprj, UP_usage);

         } else if ((userprj = userprj_list_locate(project_list, 
                              lGetPosString(node, sn_name_pos)))) {

            usage_list = lGetList(userprj, UP_usage);

         }

      }

   } else {

      /*-------------------------------------------------------------
       * If this is a project node, then return the project usage
       * rather than the children's usage
       *-------------------------------------------------------------*/

      if (!projname) {
         if ((userprj = userprj_list_locate(project_list, 
                                lGetPosString(node, sn_name_pos)))) {
            project_node = 1;
            usage_list = lGetList(userprj, UP_usage);
            projname = lGetString(userprj, UP_name);
         }
      }

   }

   if (usage_list) {

      /*-------------------------------------------------------------
       * Decay usage
       *-------------------------------------------------------------*/

      if (curr_time)
         decay_userprj_usage(userprj, decay_list, seqno, curr_time);

      /*-------------------------------------------------------------
       * Sum usage weighting factors
       *-------------------------------------------------------------*/

      if (sconf_is()) {
         usage_weight_list = sconf_get_usage_weight_list();
         if (usage_weight_list) {
            for_each(usage_weight, usage_weight_list)
               sum_of_usage_weights +=
                     lGetPosDouble(usage_weight, ua_value_pos);
         }
      }

      /*-------------------------------------------------------------
       * Combine user/project usage based on usage weighting factors
       *-------------------------------------------------------------*/

      if (usage_weight_list) {
         for_each(usage_elem, usage_list) {
            usage_name = lGetPosString(usage_elem, ua_name_pos);
            usage_weight = lGetElemStr(usage_weight_list, UA_name,
                                       usage_name);
            if (usage_weight && sum_of_usage_weights>0) {
               usage_value += lGetPosDouble(usage_elem, ua_value_pos) *
                  (lGetPosDouble(usage_weight, ua_value_pos) /
                   sum_of_usage_weights);
            }
         }
      }

      /*-------------------------------------------------------------
       * Store other usage values in node usage list
       *-------------------------------------------------------------*/

      for_each(usage_elem, usage_list) {
         const char *nm = lGetPosString(usage_elem, ua_name_pos);
         lListElem *u;
         if (strcmp(nm, USAGE_ATTR_CPU) != 0 &&
             strcmp(nm, USAGE_ATTR_MEM) != 0 &&
             strcmp(nm, USAGE_ATTR_IO) != 0) {
            if (((u=lGetElemStr(lGetPosList(node, sn_usage_list_pos),
                                UA_name, nm))) ||
                ((u = lAddSubStr(node, UA_name, nm, STN_usage_list, UA_Type))))
               lSetPosDouble(u, ua_value_pos,
                             lGetPosDouble(u, ua_value_pos) +
                             lGetPosDouble(usage_elem, ua_value_pos));
         }
      }
   }

   if (children) {
      double child_usage = 0;

      /*-------------------------------------------------------------
       * Sum child usage
       *-------------------------------------------------------------*/

      for_each(child_node, children) {
         lListElem *nu;
         child_usage += sge_calc_node_usage(child_node, user_list,
                                            project_list, decay_list, curr_time,
                                            projname, seqno);

         /*-------------------------------------------------------------
          * Sum other usage values
          *-------------------------------------------------------------*/

         if (!project_node)
            for_each(nu, lGetPosList(child_node, sn_usage_list_pos)) {
               const char *nm = lGetPosString(nu, ua_name_pos);
               lListElem *u;
               if (((u=lGetElemStr(lGetPosList(node, sn_usage_list_pos),
                                   UA_name, nm))) ||
                   ((u=lAddSubStr(node, UA_name, nm, STN_usage_list, UA_Type))))
                  lSetPosDouble(u, ua_value_pos,
                                lGetPosDouble(u, ua_value_pos) +
                                lGetPosDouble(nu, ua_value_pos));
            }
      }

      if (!project_node)

         /* if this is not a project node, we include the child usage */

         usage_value += child_usage;

      else {

         /* If this is a project node, then we calculate the usage
            being used by all users which map to the "default" user node
            by subtracting the sum of all the child usage from the
            project usage. Then, we add this usage to all of the nodes
            leading to the "default" user node. */

         lListElem *default_node;
         ancestors_t ancestors;
         int i;
         if ((default_node=search_ancestors(node, "default", &ancestors, 1))) {
            double default_usage = usage_value - child_usage;
            if (default_usage > 1.0) {
               for(i=1; i<ancestors.depth; i++) {
                  double u = lGetPosDouble(ancestors.nodes[i], sn_combined_usage_pos);
                  lSetPosDouble(ancestors.nodes[i], sn_combined_usage_pos, u + default_usage);
               }
            }
            free_ancestors(&ancestors);
         }
      }

#ifdef notdef
      else {
         lListElem *default_node;
         if ((default_node=search_named_node(node, "default")))
            lSetPosDouble(default_node, sn_combined_usage_pos,
               MAX(usage_value - child_usage, 0));
      }
#endif

   }

   /*-------------------------------------------------------------
    * Set combined usage in the node
    *-------------------------------------------------------------*/

   lSetPosDouble(node, sn_combined_usage_pos, usage_value);

   DEXIT;
   return usage_value;
}


/*--------------------------------------------------------------------
 * sge_calc_node_proportions - calculate share tree node proportions
 * for this node and all descendant nodes.
 *--------------------------------------------------------------------*/

void
sge_calc_node_proportion( lListElem *node,
                          double total_usage )
{
   lList *children;
   lListElem *child_node;
   static int sn_children_pos = -1;
   static int sn_actual_proportion_pos = -1;
   static int sn_combined_usage_pos = -1;

   DENTER(TOP_LAYER, "sge_calc_node_proportions");

   if (sn_children_pos == -1) {
      sn_children_pos = lGetPosViaElem(node, STN_children);
      sn_actual_proportion_pos = lGetPosViaElem(node, STN_actual_proportion);
      sn_combined_usage_pos = lGetPosViaElem(node, STN_combined_usage);
   }

   /*-------------------------------------------------------------
    * Calculate node proportions for all children
    *-------------------------------------------------------------*/

   if ((children = lGetPosList(node, sn_children_pos))) {
      for_each(child_node, children) {
         sge_calc_node_proportion(child_node, total_usage);
      }
   }  

   /*-------------------------------------------------------------
    * Set proportion in the node
    *-------------------------------------------------------------*/

   if (total_usage == 0)
      lSetPosDouble(node, sn_actual_proportion_pos, 0);
   else 
      lSetPosDouble(node, sn_actual_proportion_pos,
         lGetPosDouble(node, sn_combined_usage_pos) / total_usage);

   DEXIT;
   return;
}


/*--------------------------------------------------------------------
 * sge_calc_share_tree_proportions - calculate share tree node
 * usage and proportions.
 *
 * Sets STN_combined_usage and STN_actual_proportion in each share
 * tree node contained in the passed-in share_tree argument.
 *--------------------------------------------------------------------*/

void
_sge_calc_share_tree_proportions( lList *share_tree,
                                  const lList *user_list,
                                  const lList *project_list,
                                  const lList *decay_list,
                                  u_long curr_time )
{
   lListElem *root;
   double total_usage;

   DENTER(TOP_LAYER, "sge_calc_share_tree_proportions");

   if (!share_tree || !((root=lFirst(share_tree)))) {
      DEXIT;
      return;
   }

   calculate_default_decay_constant( sconf_get_halftime());

   total_usage = sge_calc_node_usage(root,
                                     user_list,
                                     project_list,
                                     decay_list,
                                     curr_time,
                    				       NULL,
                                     -1);

   sge_calc_node_proportion(root, total_usage);

   DEXIT;
   return;
}


void
sge_calc_share_tree_proportions( lList *share_tree,
                                 const lList *user_list,
                                 const lList *project_list,
                                 const lList *decay_list )
{
   _sge_calc_share_tree_proportions(share_tree, user_list, project_list,
                                    decay_list, sge_get_gmt());
   return;
}


/*--------------------------------------------------------------------
 * set_share_tree_project_flags - set the share tree project flag for
 *       node and descendants
 *--------------------------------------------------------------------*/

void
set_share_tree_project_flags( const lList *project_list,
                              lListElem *node )
{
   lList *children;
   lListElem *child;

   if (!project_list || !node)
      return;

   if (userprj_list_locate(project_list, lGetString(node, STN_name)))
      lSetUlong(node, STN_project, 1);
   else
      lSetUlong(node, STN_project, 0);

   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         set_share_tree_project_flags(project_list, child);
      }
   }
   return;
}


void
sge_add_default_user_nodes( lListElem *root_node,
                            const lList *user_list,
                            const lList *project_list,
                            const lList *userset_list)
{
   lListElem *user, *project, *pnode, *dnode;
   const char *proj_name, *user_name;

   /*
    * do for each project and for no project
    *    if default node exists
    *       do for each user
    *          if user maps to default node
    *             add temp node as sibling to default node
    *          endif
    *       end do
    *    endif
    * end do
    */

   set_share_tree_project_flags(project_list, root_node);

   for_each(project, project_list) {
      /*
      ** check acl and xacl of project for the temp users
      ** only users that are allowed for the project are shown
      */
      lList *xacl = lGetList(project, UP_xacl);
      lList *acl = lGetList(project, UP_acl);

      proj_name = lGetString(project, UP_name);

      if (search_userprj_node(root_node, "default", proj_name, NULL)) {
         for_each(user, user_list) {
            int has_access = 1;
#if 0            
            int has_usage = 1;
            lList *user_prj_usage = lGetList(user, UP_project);
#endif            
            user_name = lGetString(user, UP_name);

            /*
            ** check if user would be allowed
            */
            has_access = sge_has_access_(user_name, NULL, acl, xacl, userset_list);
#if 0            
            /*
            ** check if user has usage for this project
            */
            has_usage = (lGetElemStr(user_prj_usage, UPP_name, proj_name) != NULL);
#endif

            if (has_access && ((dnode=search_userprj_node(root_node, user_name, proj_name, &pnode))) &&
                !strcmp("default", lGetString(dnode, STN_name))) {
               lListElem *node = lCopyElem(dnode);
               lSetString(node, STN_name, user_name);
               lSetList(node, STN_children, NULL);
               lSetUlong(node, STN_temp, 1);
               if (lGetList(dnode,STN_children) == NULL) {
                  lList *children = lCreateList("display", STN_Type);
                  lSetList(dnode, STN_children, children);
               }   
               lAppendElem(lGetList(dnode,STN_children), node);
            }
         }
      }
   }

   proj_name = NULL;
   if (search_userprj_node(root_node, "default", proj_name, NULL)) {
      for_each(user, user_list) {
         user_name = lGetString(user, UP_name);
         if (((dnode=search_userprj_node(root_node, user_name, proj_name, &pnode))) &&
             strcmp("default", lGetString(dnode, STN_name)) == 0) {
            lListElem *node = lCopyElem(dnode);
            lSetString(node, STN_name, user_name);
            lSetList(node, STN_children, NULL);
            lSetUlong(node, STN_temp, 1);
            if (lGetList(dnode,STN_children) == NULL) {
               lList *children = lCreateList("display", STN_Type);
               lSetList(dnode, STN_children, children);
            }   
            lAppendElem(lGetList(dnode,STN_children), node);
         }
      }
   }

}


/********************************************************
 Search the share tree for the node corresponding to the
 user / project combination
 ********************************************************/
static lListElem *
search_userprj_node_work( lListElem *ep,      /* branch to search */
                          const char *username,
                          const char *projname,
                          lListElem **pep,    /* parent of found node */
                          lListElem *root )

{
   lListElem *cep, *fep;
   static int sn_children_pos = -1;
   static int sn_name_pos = -1;
   static int sn_project_pos = -1;
   const char *nodename;
   lList *children;

   DENTER(TOP_LAYER, "search_userprj_node");

   if (!ep || (!username && !projname)) {
      DEXIT;
      return NULL;
   }

   if (sn_name_pos == -1) {
      sn_children_pos = lGetPosViaElem(ep, STN_children);
      sn_name_pos = lGetPosViaElem(ep, STN_name);
      sn_project_pos = lGetPosViaElem(ep, STN_project);
   }

   nodename = lGetPosString(ep, sn_name_pos);

   /*
    * skip project nodes which don't match
    */

   if (lGetPosUlong(ep, sn_project_pos) &&
        ep != root &&
       (!projname || strcmp(nodename, projname))) {
      DEXIT;
      return NULL;
   }

   children = lGetPosList(ep, sn_children_pos);

   /*
    * if project name is supplied, look for the project
    */

   if (projname) {

      if (strcmp(nodename, projname) == 0) {

         /*
          * We have found the project node, now find the user node
          * within the project sub-tree. If there are no children,
          * return the project node.
          */

         if (!children) {
            DEXIT;
            return ep;
         }

         return search_userprj_node_work(ep, username, NULL, pep, ep);

#if 0

         for_each(cep, children) {
            if ((fep = search_userprj_node_work(cep, username, NULL, pep, ep))) {
               if (pep && (cep == fep))
                  *pep = ep;
               DEXIT;
               return fep;
            }
         }

         /*
          * The user is not in the project sub-tree, so look for
          * a user node called "default", which can be used to
          * specify shares for users not in the share tree.
          */

         for_each(cep, children) {
            if ((fep = search_userprj_node_work(cep, "default", NULL, pep, root))) {
               if (pep && (cep == fep))
                  *pep = ep;
               DEXIT;
               return fep;
            }
         }
#endif

         /*
          * user was not found, fall thru and return NULL
          */

      } else {

         /* 
          * search the child nodes for the project
          */

         for_each(cep, children) {
            if ((fep = search_userprj_node_work(cep, username, projname, pep, root))) {
               if (pep && (cep == fep))
                  *pep = ep;
               DEXIT;
               return fep;
            }
         }

         /*
          * project was not found, fall thru and return NULL
          */

      }

   } else {

      if (strcmp(nodename, username) == 0) {
         DEXIT;
         return ep;
      }

      /*
       * no project name supplied, so search for child node
       */

      for_each(cep, children) {
         if ((fep = search_userprj_node_work(cep, username, projname, pep, root))) {
            if (pep && (cep == fep))
               *pep = ep;
            DEXIT;
            return fep;
         }
      }

      /*
       * if we've searched the entire tree, search for default user
       */

      if (ep == root && strcmp(username, "default"))
         return search_userprj_node(ep, "default", NULL, pep);

      /*
       * user was not found, fall thru and return NULL
       */

   }

   DEXIT;
   return NULL;
}


/********************************************************
 Search the share tree for the node corresponding to the
 user / project combination
 ********************************************************/
lListElem *
search_userprj_node( lListElem *ep,      /* root of the tree */
                     const char *username,
                     const char *projname,
                     lListElem **pep )   /* parent of found node */

{
   return search_userprj_node_work(ep, username, projname, pep, ep);
}


/*--------------------------------------------------------------------
 * sgeee_sort_jobs - sort jobs according the task-priority and job number 
 *--------------------------------------------------------------------*/

void sgeee_sort_jobs( lList **job_list )              /* JB_Type */
{
   lListElem *job = NULL, *nxt_job = NULL;     
   lList *tmp_list = NULL;    /* SGEJ_Type */

   DENTER(TOP_LAYER, "sgeee_sort_jobs");

   if (!job_list || !*job_list) {
      DEXIT;
      return;
   }

#if 1
   DPRINTF(("+ + + + + + + + + + + + + + + + \n"));
   DPRINTF(("     SORTING SGEEE JOB LIST     \n"));
   DPRINTF(("+ + + + + + + + + + + + + + + + \n"));
#endif

   /*-----------------------------------------------------------------
    * Create tmp list 
    *-----------------------------------------------------------------*/
   tmp_list = lCreateList("tmp list", SGEJ_Type);

   nxt_job = lFirst(*job_list); 
   while((job=nxt_job)) {
      lListElem *tmp_sge_job = NULL;   /* SGEJ_Type */
      
      nxt_job = lNext(nxt_job);
      tmp_sge_job = lCreateElem(SGEJ_Type);

      {
         lListElem *tmp_task; /* JAT_Type */

         /* 
          * First try to find an enrolled task 
          * It will have the highest priority
          */
         tmp_task = lFirst(lGetList(job, JB_ja_tasks));

         /* 
          * If there is no enrolled task than take the template element 
          */
         if (tmp_task == NULL) {
            tmp_task = lFirst(lGetList(job, JB_ja_template));
         }

         lSetDouble(tmp_sge_job, SGEJ_priority,
                    lGetDouble(tmp_task, JAT_prio));
      }

      lSetUlong(tmp_sge_job, SGEJ_job_number, lGetUlong(job, JB_job_number));
      lSetRef(tmp_sge_job, SGEJ_job_reference, job);
#if 1
      DPRINTF(("JOB: "sge_u32" PRIORITY: "sge_u32"\n", 
         lGetUlong(tmp_sge_job, SGEJ_job_number), 
         lGetDouble(tmp_sge_job, SGEJ_priority)));
#endif
      lAppendElem(tmp_list, tmp_sge_job);
      
      lDechainElem(*job_list, job);
   }

   /*-----------------------------------------------------------------
    * Sort tmp list
    *-----------------------------------------------------------------*/
   lPSortList(tmp_list, "%I- %I+", SGEJ_priority, SGEJ_job_number);

   /*-----------------------------------------------------------------
    * rebuild job_list according sort order
    *-----------------------------------------------------------------*/
   for_each(job, tmp_list) {
      lAppendElem(*job_list, lGetRef(job, SGEJ_job_reference)); 
   } 

   /*-----------------------------------------------------------------
    * Release tmp list
    *-----------------------------------------------------------------*/
   lFreeList(tmp_list);

   DEXIT;
   return;
}


