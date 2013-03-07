;;; meta2-mode.el --- Meta-II mode

;; Derived from: snobol-mode.el (by Shae Erisson)

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; adapted from http://claystuart.blogspot.com/2012/09/a-snobol4-major-mode-for-emacs.html
;; who got it from http://emacs-fu.blogspot.com/2010/04/creating-custom-modes-easy-way-with.html

(require 'generic-x) ;; required

(define-generic-mode 'meta2-mode
  (list)                                       ;; comments
  ;; keywords
  (list)
  '(("'[^']*'"  . font-lock-string-face)    ;; highlights labels
    ("\\$\\|=\\|/\\||\\|;\\|\\*\\|\\.,\\|<\\|>" . font-lock-builtin-face) ;; highlights operators
    ("\\.\\(OUT\\|LENGTH\\|STRING\\|NUMBER\\|ID\\|LABEL\\|SYNTAX\\|END\\)\\>"
     . font-lock-keyword-face))
  '("\\.meta2$")                                     ;; file endings
   nil                                             ;; other function calls
  "A mode for Meta-II files"                       ;; doc string
)

(add-hook 'meta2-mode-hook
          (lambda ()
	    (setq font-lock-defaults (list 'generic-font-lock-defaults nil t))
            (set (make-local-variable 'compile-command)
                 (concat "meta2 <" buffer-file-name))))

(add-to-list 'auto-mode-alist '("\\.meta2$" . meta2-mode))

(provide 'meta2-mode)
