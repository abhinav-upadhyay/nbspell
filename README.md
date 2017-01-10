spell
=====

The first version of spell(1) came in 1975 and was based on affix and prefix
rules, which have not proven to be very accurate. Moreover, faster and better
techniques have evolved over the decades. For example edit distance based
techniques are capable of figuring out about 90% of spelling errors[1]. For
more difficult errors which depend on the context, there are more
sophisticated models such as n-gram[2]. The SUS has marked spell(1) as
legacy because of its inability to offer language sensitive spell checking [3].
Even though language detection is a hard problem, however, the n-gram based model
could be generalized to detect languages as well[4]. 

This implementation proposes to overcome the above mentioned problems along with
the aim of offering a comparison of the trade offs of the modern techniques in
terms of their accuracy and simplicity in terms of ease of implementation and maintenance.


###  References ###
  1. http://norvig.com/spell-correct.html
  2. Kukich, Techniques for Automatically Correcting Words in Text; ACM Comput.
      Surv. 1992 (http://www.devl.fr/docs/these/bibli/Kukich1992Techniqueforautomatically.pdf)
  3. http://pubs.opengroup.org/onlinepubs/007908799/xcu/spell.html
  4. Cavnar and Trenkle, N-Gram-Based Text Categorization; In Proceedings of SDAIR-94
