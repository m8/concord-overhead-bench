#line 270 "../../../../splash2/codes//null_macros/c.m4.null"

#line 1 "grav.H"
#ifndef _GRAV_H_
#define _GRAV_H_

void hackgrav(bodyptr p, long ProcessId);
void gravsub(register nodeptr p, long ProcessId);
void hackwalk(long ProcessId);
void walksub(nodeptr n, real dsq, long ProcessId);
bool subdivp(register nodeptr p, real dsq, long ProcessId);

#endif
