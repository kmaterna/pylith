c -*- Fortran -*-
c
c~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
c
c                             Charles A. Williams
c                       Rensselaer Polytechnic Institute
c                        (C) 2004  All Rights Reserved
c
c  Copyright 2004 Rensselaer Polytechnic Institute.
c  All worldwide rights reserved.  A license to use, copy, modify and
c  distribute this software for non-commercial research purposes only
c  is hereby granted, provided that this copyright notice and
c  accompanying disclaimer is not modified or removed from the software.
c
c  DISCLAIMER:  The software is distributed "AS IS" without any express
c  or implied warranty, including but not limited to, any implied
c  warranties of merchantability or fitness for a particular purpose
c  or any warranty of non-infringement of any current or pending patent
c  rights.  The authors of the software make no representations about
c  the suitability of this software for any particular purpose.  The
c  entire risk as to the quality and performance of the software is with
c  the user.  Should the software prove defective, the user assumes the
c  cost of all necessary servicing, repair or correction.  In
c  particular, neither Rensselaer Polytechnic Institute, nor the authors
c  of the software are liable for any indirect, special, consequential,
c  or incidental damages related to the software, to the maximum extent
c  the law permits.
c
c~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
c
c
      subroutine getjac(x,xs,det,shj,nen,iel,ierr,errstrng)
c
c...  subroutine to compute the jacobian determinant given the element
c     coordinates and the shape functions in natural coordinates.
c
c       shj(1,nen),sh(2,nen),sh(3,nen) = x,y,and z derivatives
c                                        of shape functions
c       xs(nsd,nsd)                   = jacobian matrix
c       det                           = determinant of jacobian matrix
c       x(nsd,nen)                    = local nodal coordinates
c
      include "implicit.inc"
c
c...  parameter definitions
c
      include "ndimens.inc"
      include "nshape.inc"
      include "nconsts.inc"
      include "rconsts.inc"
c
c...  subroutine arguments
c
      integer nen,iel,ierr
      character errstrng*(*)
      double precision x(nsd,nen),xs(nsd,nsd),det,shj(nsd+1,nenmax)
c
c...  local variables
c
cdebug      integer idb,jdb
c
cdebug      write(6,*) "Hello from getjac_f!"
c
      ierr=izero
c
c...calculate jacobian matrix for (x,y,z) to (r,s,t) transformation
c
cdebug      call dgemm("n","t",nsd,nsd,nen,one,shj,nsd+1,x,nsd,zero,xs,nsd)
cdebug      do idb=1,nen
cdebug        write(6,*) idb,(x(jdb,idb),jdb=1,nsd)
cdebug      end do
      call dgemm("n","t",nsd,nsd,nen,one,x,nsd,shj,nsd+ione,zero,xs,nsd)
c
c...form determinant of jacobian matrix and check for error condition
c
      det=xs(1,1)*xs(2,2)*xs(3,3)+xs(1,2)*xs(2,3)*xs(3,1)+xs(1,3)
     & *xs(2,1)*xs(3,2)-xs(1,3)*xs(2,2)*xs(3,1)-xs(1,2)*xs(2,1)
     & *xs(3,3)-xs(1,1)*xs(2,3)*xs(3,2)
cdebug      write(6,*) "iel,det:",iel,det
cdebug      do idb=1,nen
cdebug        write(6,*) "idb,x:",idb,(x(jdb,idb),jdb=1,nsd)
cdebug      end do
cdebug      do idb=1,nsd
cdebug        write(6,*) "idb,xs:",idb,(xs(jdb,idb),jdb=1,nsd)
cdebug      end do
cdebug      do idb=1,nen
cdebug        write(6,*) "idb,shj:",idb,(shj(jdb,idb),jdb=1,nsd+1)
cdebug      end do
cdebug      call flush(6)
      if(det.le.zero) then
        ierr=113
cdebug        write(6,700) iel,det
        write(errstrng,700) iel,det
      end if
c
 700  format("getjac:  element # ",i7,2x,1pe15.8)
      return
      end
c
c version
c $Id: getjac.f,v 1.7 2005/02/24 00:00:13 willic3 Exp $
c
c Generated automatically by Fortran77Mill on Wed May 21 14:15:03 2003
c
c End of file 
