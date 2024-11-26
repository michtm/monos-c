/**
 * @file error.c
 * @brief Implémentation de error.h
 * @author michtm
 * @date 2024-11-09
 * @see error.h
 */
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"

/** Variables */
const struct monos_except monos_assert_error = { "Assertion failed" };

static struct monos_except_context ctxt_0;
static struct monos_except_context *ctxt_stack_p = NULL;

/** Fonctions */
void
monos_except_throw_loc(const struct monos_except *except_p,
                       const char *file,
                       const char *func,
                       unsigned int line)
{
  struct monos_except_context *ctxt_p = NULL;
  struct monos_except_context *x_ctxt_p = NULL;
  int i = 0;
  int context_found = 0;

  /* Recherche de l'exception à lever */
  for (x_ctxt_p = ctxt_stack_p; x_ctxt_p != NULL; x_ctxt_p = x_ctxt_p->link_p)
    {
      for (i = 0; i < x_ctxt_p->except_count && !context_found; ++i)
        {
          context_found = (x_ctxt_p->array[i] == except_p);
        }
      if (context_found) break;
    }

  /* Si l'exception n'est pas trouvée, sortir en erreur */
  if (x_ctxt_p == NULL)
    {
      if (file && func && line > 0)
        (void) fprintf(stderr, "%s:%s:%u: ", file, func, line);
      (void) fprintf(stderr, "Uncaught exception");
      if (except_p->message)
        (void) fprintf(stderr, " `%s`\n", except_p->message);
      else
        (void) fprintf(stderr, "@%p\n", (void *) except_p);
      (void) fflush(stderr);
      abort();
    }

  /* Recherche du contexte où se trouverait le bloc Finally */
  for (ctxt_p = ctxt_stack_p;
       ctxt_p != NULL && ctxt_p != x_ctxt_p && !(ctxt_p->has_finally);
       ctxt_p = ctxt_p->link_p);

  ctxt_stack_p = ctxt_p;
  ctxt_p->except_p = except_p;
  ctxt_p->file = file;
  ctxt_p->func = func;
  ctxt_p->line = line;
  longjmp(ctxt_p->env, EXCEPT_THROWN);
}

void
monos_except_context_push(struct monos_except_context *ctxt_p)
{
  /* Initialisation de la pile au besoin */
  if (ctxt_stack_p == NULL) ctxt_stack_p = &ctxt_0;
  /* Opération Push */
  ctxt_p->link_p = ctxt_stack_p;
  ctxt_stack_p = ctxt_p;
}

void
monos_except_context_pop(struct monos_except_context *ctxt_p)
{
  ctxt_stack_p = ctxt_p->link_p;
}

void
(monos_assert)(int e)
{
  monos_assert(e);
}
