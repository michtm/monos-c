/**
 * @file error.h
 * @brief Librairie de gestion des erreurs (assertions et exceptions, voir plus)
 * @author michtm
 * @date 2024-11-09
 * @see error.c
 * @see Eric S; Roberts: Implementing Exceptions in C
 * @todo Gestion des signaux UNIX: les tourner en exceptions
 * @todo Implémenter une fonction qui affiche le contenu de la pile d'exceptions
 * @fixme Le bloc Finally ne fonctionne pas lorsqu'une exception levée n'est pas
 * gérée
 *
 * - Pour les exceptions:
 *
 * throw(some_exception, ...) Lève une exception
 *
 * try { ... } Exécute des instructions susceptibles de produire des erreurs ou
 * des exceptions
 *
 * catch(some_exception) { ... } Exécute des instructions lorsqu'une exception
 * est capturée
 *
 * finally { ... } Exécute des instructions après le bloc try, dans tous les cas
 *
 * end_try Permet de terminer correctemet le flot de contrôle d'exceptions
 *
 * - Pour les assertions:
 *
 * assert(expr) Teste une expression qui, en cas d'échec, lève une exception et
 * arrête l'exécution du programme par abort()
 *
 */
#ifndef MONOS_ERROR_H
# define MONOS_ERROR_H 1

# include <setjmp.h>
# include <stddef.h>
# include <stdlib.h>

# define MONOS_MAX_EXCEPT 32

/** Types */
struct monos_except
{
  const char *message; /**< Des détails sur l'exception */
};

struct monos_except_context
{
  jmp_buf env;
  int except_count;
  const struct monos_except *array[MONOS_MAX_EXCEPT];
  const struct monos_except *except_p;
  const char *file;
  const char *func;
  unsigned int line;
  int has_finally;
  struct monos_except_context *link_p;
};

/** Etats du flot de contrôle d'exceptions */
enum monos_except_state {
  EXCEPT_INIT = 0,
  EXCEPT_BODY,
  EXCEPT_THROWN,
  EXCEPT_HANDLED,
  EXCEPT_END
};

/** Variables globales */
extern const struct monos_except monos_assert_error;

/** Fonctions */
extern void
monos_except_throw_loc(const struct monos_except *except_p,
                       const char *file, const char *func, unsigned int line);

extern void
monos_except_context_push(struct monos_except_context *ctxt_p);

extern void
monos_except_context_pop(struct monos_except_context *ctxt_p);

extern void
monos_assert(int e);

/** Macros */
# undef assert

# define throw(e) monos_except_throw_loc(&e, __FILE__, __func__, __LINE__)
# define try monos_try
# define catch(e) monos_catch(e)
# define finally monos_finally
# define end_try monos_end_try
# define assert(expr) monos_assert(expr)

# define monos_try \
  { \
    /* Initialisation du contexte local */ \
    volatile enum monos_except_state _monos_except_state = EXCEPT_INIT; \
    struct monos_except_context _monos_except_ctxt; \
    _monos_except_ctxt.except_count = 0; \
    _monos_except_ctxt.except_p = NULL; \
    _monos_except_ctxt.file = NULL; \
    _monos_except_ctxt.func = NULL; \
    _monos_except_ctxt.line = 0; \
    _monos_except_ctxt.has_finally = 0; \
    /* Ajout du contexte dans une pile */ \
    monos_except_context_push(&_monos_except_ctxt); \
    /* Sauvegarde de l'environnement par setjmp */ \
    if (setjmp(_monos_except_ctxt.env) != 0) _monos_except_state = EXCEPT_THROWN; \
    /* Boucle infinie: pour gérer le flot de contrôle d'exceptions */ \
    while (_monos_except_state != EXCEPT_END) \
      { \
        /* Evaluation du bloc d'instructions utilisateur */ \
        if (_monos_except_state == EXCEPT_BODY) \
          {

# define monos_catch(e) \
            /* Cas où tout se passe bien: Suppression du contexte depuis une pile */ \
            if (_monos_except_state == EXCEPT_BODY) \
              monos_except_context_pop(&_monos_except_ctxt); \
            _monos_except_state = EXCEPT_END; \
          } \
          /* Premier tour de boucle: Ajout de l'exception dans un tableau */ \
          if (_monos_except_state == EXCEPT_INIT) \
            { \
              if (_monos_except_ctxt.except_count >= MONOS_MAX_EXCEPT) \
                exit(EXIT_FAILURE); \
              _monos_except_ctxt.array[_monos_except_ctxt.except_count++] = &(e); \
            } \
          /* Exception trouvée: Evaluation du bloc d'instructions utilisateur */ \
          else if (_monos_except_ctxt.except_p == &(e)) \
            { \
              _monos_except_state = EXCEPT_HANDLED; \
              monos_except_context_pop(&_monos_except_ctxt);

# define monos_finally \
          } \
          /* Premier tour de boucle: Indique la présence du bloc Finally */ \
          if (_monos_except_state == EXCEPT_INIT) \
            { \
              if (_monos_except_ctxt.except_count >= MONOS_MAX_EXCEPT) \
                exit(EXIT_FAILURE); \
              _monos_except_ctxt.has_finally = 1; \
            } \
          /* Dans tous les cas: Evaluation du bloc d'instructions utilisateur */ \
          else \
            { \
              monos_except_context_pop(&_monos_except_ctxt);

# define monos_end_try \
            /* En présence du bloc Finally, lève à nouveau l'exception non gérée */ \
            if (_monos_except_ctxt.has_finally && _monos_except_state == EXCEPT_THROWN) \
              monos_except_throw_loc(_monos_except_ctxt.except_p, \
                                     _monos_except_ctxt.file, \
                                     _monos_except_ctxt.func, \
                                     _monos_except_ctxt.line); \
            _monos_except_state = EXCEPT_END; \
          } \
        /* Premier tour de boucle: évaluation du bloc Try */ \
            if (_monos_except_state != EXCEPT_END) _monos_except_state = EXCEPT_BODY; \
      } \
  }

# define monos_assert(expr) \
  ((expr) \
   ? (void) 0 \
   : throw(monos_assert_error))

#endif /* !MONOS_ERROR_H */
