/*
 * main.c
 *
 * Test application for tinysh module
 *
 * Copyright (C) 2001 Michel Gutierrez <mig@nerim.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "tinysh.h"

/*--- stuff for single character input entry --------------------------------*/
static struct termios saved_term_state;

int set_cbreak (int fd)
{
  struct termios temp_term_state;
  /* get current terminal state */
  if (tcgetattr (fd, &saved_term_state) < 0)
    {
      perror ("could not get terminal state : ");
      return (errno);
    }
  temp_term_state = saved_term_state;
  temp_term_state.c_lflag &= ~(ECHO | ICANON);
  temp_term_state.c_cc [VMIN] = 1;    /* return after 1 byte min */
  temp_term_state.c_cc [VTIME] = 0;   /* wait forever for data */
  /* set terminal state to cbreak */
  if (tcsetattr (fd, TCSAFLUSH, &temp_term_state) < 0)
    {
      perror ("could not set terminal state : ");
      return (errno);
    }
  return (0);
}

static int unset_cbreak(int fd)
{
  if (tcsetattr (fd, TCSAFLUSH, &saved_term_state) < 0)
    {
      perror ("could not set terminal state : ");
      return (errno);
    }
  return 0;
}

/*--------------------------------------------------------------------------*/

/* we must provide this function to use tinysh
 */
void tinysh_char_out(unsigned char c)
{
  putchar((int)c);
}

static void display_args(int argc, char **argv)
{
  int i;
  for(i=0;i<argc;i++)
    {
      printf("argv[%d]=\"%s\"\n",i,argv[i]);
    }
}

static void foo_fnt(int argc, char **argv)
{
  printf("foo command called\n");
  display_args(argc,argv);
}

static tinysh_cmd_t myfoocmd={0,"foo","foo command","[args]",
                              foo_fnt,0,0,0};

static void item_fnt(int argc, char **argv)
{
  printf("item%d command called\n",(int)tinysh_get_arg());
  display_args(argc,argv);
}

static tinysh_cmd_t ctxcmd={0,"ctx","contextual command","item1|item2",
                            0,0,0,0};
static tinysh_cmd_t item1={&ctxcmd,"item1","first item","[args]",item_fnt,
                           (void *)1,0,0};
static tinysh_cmd_t item2={&ctxcmd,"item2","second item","[args]",item_fnt,
                           (void *)2,0,0};

static void reset_to_0(int argc, char **argv)
{
  int *val;
  val=(int *)tinysh_get_arg();
  *val=0;
}

static void atoxi_fnt(int argc, char **argv)
{
  int i;

  for(i=1;i<argc;i++)
    {
      printf("\"%s\"-->%u (0x%x)\n",
             argv[i],tinysh_atoxi(argv[i]),tinysh_atoxi(argv[i]));
    }
}

static tinysh_cmd_t atoxi_cmd={0,"atoxi","demonstrate atoxi support",
                               "[args-to-convert]",atoxi_fnt,0,0,0};

int main(int argc, char **argv)
{
  int c;
  int again=1;

/* change the prompt */
  tinysh_set_prompt(PACKAGE "-" VERSION "$ ");

/* add the foo command 
*/
  tinysh_add_command(&myfoocmd);

/* add sub commands
 */
  tinysh_add_command(&ctxcmd);
  tinysh_add_command(&item1);
  tinysh_add_command(&item2);

/* use a command from the stack
 * !!! this is only possible because the shell will have exited
 * before the stack of function main !!!
 */
  {
    tinysh_cmd_t quitcmd={0,"quit","exit shell",0,reset_to_0,
                          (void *)&again,0,0};
    tinysh_add_command(&quitcmd);    
  }

/* add atoxi support test command */
  tinysh_add_command(&atoxi_cmd);

/* remove standard input buffering to simulate minimal input capability
 * like read from the uart in an embedded environment
 */
  set_cbreak(0);

/* main loop
 */
  while(again)
    {
      c=getchar();
      tinysh_char_in((unsigned char)c);
    }
  printf("\nBye\n");
  unset_cbreak(0);
}

