#
# Ce Makefile contient les cibles suivantes :
#
# all		: compile les programmes
# clean		: supprime les fichiers générés automatiquement
# coverage	: compile les programmes pour mesurer la couverture de code
# test		: lance tous les tests (scripts shell test-xxx.sh)
#		  (sans appeler valgrind)
# gcov		: génère les rapports de couverture (à lancer après avoir
#		  lancé les cibles 'coverage' et 'test').
#		  Résultats dans *.gcov
# ctags		: génère un fichier tags pour la navigation avec vim.
#		  (voir http://usevim.com/2013/01/18/tags/)
#
# De plus, les cibles supplémentaires suivantes sont fournies pour
# simplifier les tâches répétitives :
#
# test-avec-valgrind	: lance les tests avec valgrind (conseillé)
# couverture-et-tests	: automatise les tests avec rapport de couverture
#


CFLAGS = -g -Wall -Wextra -Werror $(COVERAGE)

PROGS = client server 

all: $(PROGS)


$(PROGS): dht.o

musee.o: dht.h

coverage: clean
	$(MAKE) COVERAGE=$(COV)

gcov:
	gcov *.c


ctags:
	ctags *.[ch]

clean:
	rm -f *.o $(PROGS)
	rm -f *.gc*
	rm -f *.log
	rm -f tags core
