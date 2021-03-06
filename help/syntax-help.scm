(define (syntax-read fn)
  (let ((p (open-input-file fn))
        (n 0)
        productions
        )
    (let (exp)
      (set! exp (read p))
      (while (not (eof-object? exp))
             (set! productions (cons exp productions))
             (set! exp (read p))))
    (close-port p)
    productions
    ))

(define (syntax-find <func>)
  (let ((p (open-input-file "help/syntax.scm"))
        matches)
    (let (exp)
      (set! exp (read p))
      (while (not (eof-object? exp))
             (if (eq? (caar exp) <func>)
                 (set! matches (cons exp matches)))
             (set! exp (read p)))
      (close-port p)
      matches)))

(define (help <func>)
  (let ((x (syntax-find <func>)))
    (if x
        (begin 
          (for-each (lambda (x)
                      (display (car x))
                      (display " -> ")
                      (display (caddr x))
                      (newline))
                    (reverse x))
          #t)
        #f)))

(define (grep^ pattern)
  (if (symbol? pattern)
      (set! pattern (symbol->string pattern)))
  (let ((pattern (string-append "^((" pattern)))
    (grep pattern)))

(define (grep pattern)
  (if (symbol? pattern)
      (set! pattern (symbol->string pattern)))
  (let ((cmd (string-append "grep \"" pattern "\" help/syntax.scm")))
    (print cmd)
    (system cmd)
    ))

(define (syntax-show)
  (let ((productions (syntax-read "help/syntax.scm")))
    (for-each print productions)))

