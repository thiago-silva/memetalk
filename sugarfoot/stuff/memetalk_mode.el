;; M-x toggle-debug-on-error

(defvar memetalk-mode-hook nil)

; "If your keymap will have very few entries, then you may want to consider
; ‘make-sparse-keymap’ rather than ‘make-keymap’."
(defvar memetalk-mode-map
  (let ((map (make-keymap)))
    (define-key map (kbd "s-d") 'memetalk-do-it)
    (define-key map (kbd "s-p") 'memetalk-print-it)
    (define-key map (kbd "s-i") 'memetalk-step-into)
    (define-key map (kbd "s-q") 'memetalk-step-over)
    (define-key map (kbd "s-u") 'memetalk-step-out)
    (define-key map (kbd "s-l") 'memetalk-step-line)
    (define-key map (kbd "s-c") 'memetalk-continue)
    (define-key map (kbd "s-r") 'memetalk-run-until)
    (define-key map (kbd "s-f") 'memetalk-reload-frame)
    (define-key map (kbd "s-e") 'memetalk-return-value)
    (define-key map (kbd "s-w") 'memetalk-recompile)
    (define-key map (kbd "s-b") 'memetalk-break-at)
    (define-key map (kbd "<s-down>") 'memetalk-bt-up)
    (define-key map (kbd "<s-up>") 'memetalk-bt-down)
    (define-key map (kbd "s-v") 'memetalk-locals)
    (define-key map (kbd "s-m") 'memetalk-toggle-module-step-mode)
    map)
  "Keymap for memetalk major mode")


(add-to-list 'auto-mode-alist '("\\.mm\\'" . memetalk-mode))


(setq memetalk-keywords '("preamble" "code" "endcode" "class" "fields"
                          "init" "fun" "super" "instance_method" "class_method"
                          "if" "elif" "else" "try" "end" "catch" "var" "primitive" "return"))

(setq memetalk-constants '("true" "false" "null"))

(setq memetalk-keywords-regexp (regexp-opt memetalk-keywords 'words))
(setq memetalk-constants-regexp (regexp-opt memetalk-constants 'words))

;; font-lock-defaults
;; js--font-lock-keywords-3 js--font-lock-keywords-1 js--font-lock-keywords-2 js--font-lock-keywords-3))

;;TODO:
;; @vars
;; this
;; thisModule
;; thisContext

(defconst memetalk-font-lock-keywords
  (list
   `(,memetalk-keywords-regexp . font-lock-keyword-face)
   `(,memetalk-constants-regexp . font-lock-constant-face)
   '("^\\s-*instance_method\\s-+\\([a-zA-Z_$]\\(?:\\s_\\|\\sw\\)*\\)" 1 font-lock-variable-name-face)
   '("^\\([a-zA-Z_$]\\(?:\\s_\\|\\sw\\)*\\):" 1 font-lock-function-name-face)
   '("var\\s-+\\([a-zA-Z_$]\\(?:\\s_\\|\\sw\\)*\\)" 1 font-lock-variable-name-face)
  "Highlighting expressions for memetalk mode"))

;; I believe the following is for c++ style comments
(defvar memetalk-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?_ "w" st)
    (modify-syntax-entry ?/ ". 124b" st)
    (modify-syntax-entry ?* ". 23" st)
    (modify-syntax-entry ?\n "> b" st)
    st)
  "Syntax table for memetalk-mode")

(defun memetalk-mode ()
  "Major mode for editing Workflow Process Description Language files"
  (interactive)
  (kill-all-local-variables)
  (set-syntax-table memetalk-mode-syntax-table)
  (use-local-map memetalk-mode-map)
  (set (make-local-variable 'font-lock-defaults) '(memetalk-font-lock-keywords))
  ;; (set (make-local-variable 'indent-line-function) 'memetalk-indent-line)
  (setq major-mode 'memetalk-mode)
  (setq mode-name "memetalk")
  (run-hooks 'memetalk-mode-hook))

;; utils

(defun memetalk-trim-r (string)
  (when (string-match "[ \t\n]*$" string)
    (replace-match "" nil nil string)))

(defun memetalk-module-name-from-path (filepath)
  (file-name-nondirectory (file-name-sans-extension filepath)))

;; socket

(defvar memetalk-repl-recon-timer nil)

(defun memetalk-repl-setup-reconnection ()
  (unless memetalk-repl-recon-timer
    (setq memetalk-repl-recon-timer (run-at-time "1 second" 1 'memetalk-repl-connect))))

(defun memetalk-repl-clear-reconnection-timer ()
  (cancel-timer memetalk-repl-recon-timer)
  (setq memetalk-repl-recon-timer nil))

(defun memetalk-repl-detect-disconnect (process event)
  (message (format "%s %s" process (memetalk-trim-r event)))
  (memetalk-clear-current-buffer)
  (memetalk-repl-setup-reconnection))


(defun memetalk-repl-is-open ()
  (and (process-status "mm-repl") t))

;; (defun memetalk-repl-last-response ()
;;   (if (memetalk-repl-is-open)
;;       (with-current-buffer "*mm-repl*"
;;         (memetalk-trim-r (thing-at-point 'line)))
;;     nil))

(defun memetalk-repl-send (cmd)
  (if (memetalk-repl-is-open)
      (process-send-string "*mm-repl*" (concat cmd "\0"))
    nil))


;; repl high level commands
(defun memetalk-repl-load (module-name)
  (memetalk-repl-send (concat "load " module-name)))

(defun memetalk-repl-do-it (code)
  (memetalk-repl-send (concat "do-it " (base64-encode-string code t))))

(defun memetalk-repl-print-it (code)
  (memetalk-repl-send (concat "print-it " (base64-encode-string code t))))

(defun memetalk-repl-return-value (code)
  (memetalk-repl-send (concat "return-value " (base64-encode-string code t))))

(defun memetalk-repl-run-until (loc)
  (memetalk-repl-send (concat "run-until " (base64-encode-string loc t))))

(defun memetalk-repl-break-at (loc)
  (memetalk-repl-send (concat "break-at " (base64-encode-string loc t))))

(defun memetalk-repl-recompile (text)
  (memetalk-repl-send
   (format "recompile %d|%s"
           (memetalk-get-begin-procedure-line) (base64-encode-string text t))))


(defvar memetalk-current-module-filepath nil)

;;; operations

(defun memetalk-repl-connect ()
  (interactive)
  (setq mm-repl-socket nil)
  (condition-case nil
      (progn
        (setq mm-repl-socket (open-network-stream "mm-repl"
                                                  "*mm-repl*" "localhost" 4200))
        (set-process-sentinel mm-repl-socket 'memetalk-repl-detect-disconnect)
        (memetalk-repl-clear-reconnection-timer)
        (message "memetalk repl connected"))
        ;; (if memetalk-current-module-filepath
        ;;     (memetalk-instantiate-module memetalk-current-module-filepath)))
    (error nil))
  mm-repl-socket)

;; (defun memetalk-work-on-current-module ()
;;   (interactive)
;;   (memetalk-instantiate-module (buffer-file-name)))

(defun memetalk-do-it ()
  (interactive)
  (let ((code (memetalk-current-selection)))
    (when (stringp code)
      (memetalk-repl-do-it code)
      (deactivate-mark))))

(defun memetalk-print-it ()
  (interactive)
  (let ((code (memetalk-current-selection)))
    (when (stringp code)
      (memetalk-repl-print-it code))))

(defun memetalk-step-into ()
  (interactive)
  (memetalk-repl-send "step-into"))

(defun memetalk-step-over ()
  (interactive)
  (memetalk-repl-send "step-over"))

(defun memetalk-step-line ()
  (interactive)
  (memetalk-repl-send "step-line"))

(defun memetalk-step-out ()
  (interactive)
  (memetalk-repl-send "step-out"))

(defun memetalk-continue ()
  (interactive)
  (memetalk-repl-send "continue"))

(defun memetalk-locals ()
  (interactive)
  (memetalk-repl-send "locals"))

(defun memetalk-bt-up ()
  (interactive)
  (memetalk-repl-send "bt-up"))

(defun memetalk-bt-down ()
  (interactive)
  (memetalk-repl-send "bt-down"))

(defun memetalk-run-until ()
  (interactive)
  (memetalk-repl-run-until (memetalk-current-location)))

(defun memetalk-reload-frame ()
  (interactive)
  (memetalk-repl-send "reload-frame"))

(defun memetalk-recompile ()
  (interactive)
  (memetalk-repl-recompile (memetalk-get-last-procedure)))

(defun memetalk-return-value ()
  (interactive)
  (let ((code (memetalk-current-selection)))
    (when (stringp code)
      (memetalk-repl-return-value code)
      (deactivate-mark))))

(defun memetalk-break-at ()
  (interactive)
  (memetalk-repl-break-at (memetalk-current-location)))

(defun memetalk-toggle-module-step-mode ()
  (interactive)
  (memetalk-repl-send "toggle-module-step-mode"))


;; utils
(defun memetalk-current-selection ()
  (condition-case nil
      (with-current-buffer (current-buffer)
        (buffer-substring-no-properties (mark) (point)))
    (error nil)))

;; (defun memetalk-instantiate-module (filepath)
;;   (setq memetalk-current-module-filepath filepath)
;;   (when (memetalk-repl-is-open)
;;     (let ((module-name (memetalk-module-name-from-path filepath)))
;;       (memetalk-repl-load module-name))))

;; hooks

;; (defun memetalk-after-save ()
;;   (when (equal (buffer-file-name) memetalk-current-module-filepath)
;;     (memetalk-instantiate-module memetalk-current-module-filepath)))


(defun memetalk-current-buffer-module-name ()
  (car (split-string (buffer-name) "\\.")))

(defun memetalk-current-location ()
  (interactive)
  (with-current-buffer (current-buffer)
    (condition-case nil
        (save-excursion
          (let ((current-line (line-number-at-pos (point)))
                (current-pos (point)))
            (re-search-backward
             "^\\s-*\\(init\\s-+\\|instance_method\\s-+\\|class_method\\s-+\\|\\)\\(\\w+\\): fun")
            (let ((fname (match-string-no-properties 2))
                  (ftype (match-string-no-properties 1))
                  (begin (match-beginning 0)))
              (cond
               ((equal ftype "")
                (format "%s:%s@%s"
                        (memetalk-current-buffer-module-name)
                        fname
                        current-line))
               ((or (equal ftype "instance_method ")
                    (equal ftype "class_method ")
                    (equal ftype "init "))
                (re-search-backward "^class \\(\\w+\\)")
                (let ((class-name (match-string-no-properties 1)))
                  (format "%s/%s:%s@%s"
                          (memetalk-current-buffer-module-name)
                          class-name
                          fname
                          current-line)))
               (t
                (message "unknown ftype"))))))
      ((debug error) nil))))


(defvar memetalk-response-content "")

(defvar memetalk-current-buffer nil)

(defun change-memetalk-current-buffer (buffer)
  (when (buffer-live-p memetalk-current-buffer)
    (memetalk-remove-overlays memetalk-current-buffer))
  (setq memetalk-current-buffer buffer))

(defun memetalk-clear-current-buffer ()
  (memetalk-remove-overlays memetalk-current-buffer)
  (setq memetalk-current-buffer nil))

(defvar memetalk-path (or  (getenv "MEME_PATH") "/Users/thiago/src/memetalk/sugarfoot/"))

(defun memetalk-get-module-file-path (file-name)
  (let (res)
    (dolist (prefix (parse-colon-path memetalk-path) res)
      (let ((path (cond
                   ((string-match ".*/\$" prefix) prefix)
                   (t (concat prefix "/")))))
        (let ((full-path (concat path file-name))
              (test-path (concat path "../tests/" file-name)))
          (unless res
            (setq res (cond
                       ((file-exists-p full-path) full-path)
                       ((file-exists-p test-path) test-path)
                       (t nil)))))))
    (if (null res)
        (error (format "can't open file %s" file-name))
      res)))

(defvar commands (list))

(defun memetalk-process-module-response (res file-part)
  (string-match "\\([a-zA-Z0-9_]+\\)[:/]" file-part)
  (let ((mod-name (concat (match-string-no-properties 1 file-part) ".mm")))
    (message (format "mattched module '%s', path: %s" mod-name (memetalk-get-module-file-path mod-name)))
    (let ((mod-path (memetalk-get-module-file-path mod-name)))
      (with-current-buffer (find-file mod-path)
        (message (format "opened file '%s'" mod-path))
        (widen) ;this shit keep being opened narrowed, god damn it!
        (change-memetalk-current-buffer (current-buffer))))
    (message resp)
    (string-match
     "@\\[\\([0-9]+\\), \\([0-9]+\\), \\([0-9]+\\), \\([0-9]+\\)\\]"
     resp)
    (let ((start-line (string-to-number (match-string-no-properties 1 resp)))
          (start-col (string-to-number (match-string-no-properties 2 resp)))
          (end-line (string-to-number (match-string-no-properties 3 resp)))
          (end-col (string-to-number (match-string-no-properties 4 resp))))
      (with-current-buffer memetalk-current-buffer
        (memetalk-remove-overlays memetalk-current-buffer)
        (goto-line  (+ 1 start-line))
        (forward-char start-col)
        (let ((begin (point)))
          (goto-line (+ 1 end-line))
          (forward-char end-col)
          (let ((x (make-overlay begin (point))))
            (overlay-put x 'face '(:box "yellow"))))))))

(defun memetalk-process-show-response (resp b64text)
  (with-current-buffer memetalk-current-buffer
    (deactivate-mark)
    (set-mark-command nil)
    (insert (base64-decode-string b64text))))

(defun memetalk-process-locals-response (resp b64text)
  (let ((base64-decode-string b64text))
    (message (base64-decode-string b64text))))

(defun memetalk-repl-dispatch-response (resp)
  (message (format "received response '%s'" resp))
  (setq commands (cons resp commands))
  (when (equal (string-match "module: \\(.*\\)@" resp) 0)
    (memetalk-process-module-response resp (match-string-no-properties 1 resp)))
  (when (equal (string-match "show: \\(.*\\)" resp) 0)
    (memetalk-process-show-response resp (match-string-no-properties 1 resp)))
  (when (equal (string-match "locals: \\(.*\\)" resp) 0)
      (memetalk-process-locals-response resp (match-string-no-properties 1 resp))))



(defun memetalk-repl-change (begin end x)
  (condition-case nil
      (when (equal (buffer-name) "*mm-repl*")
        (let ((content (buffer-substring-no-properties begin end)))
          (setq memetalk-response-content
                (concat memetalk-response-content content))
          (when (equal (string-match ".*\n" memetalk-response-content) 0)
            (memetalk-repl-dispatch-response
             (memetalk-trim-r memetalk-response-content))
            (setq memetalk-response-content ""))))
    ((debug error t))))

;; (remove-hook 'after-change-functions 'memetalk-after-change)
;; (add-hook 'after-change-functions 'memetalk-after-change)

(add-hook
 'after-change-functions 'memetalk-repl-change)

(provide 'memetalk-mode)

(defun memetalk-remove-overlays (buffer)
  (when (buffer-live-p buffer)
    (with-current-buffer buffer
      (remove-overlays (point-min) (point-max)))))

(defun memetalk-get-last-procedure () ;works only with module functions for now
  (interactive)
  (with-current-buffer (current-buffer)
    (condition-case nil
        (save-excursion
          (re-search-backward
           "^\\s-*\\(init\\s-+\\|instance_method\\s-+\\|class_method\\s-+\\|\\)\\(\\w+\\): fun")
          (let ((fname (match-string-no-properties 2))
                (ftype (match-string-no-properties 1))
                (begin (match-beginning 0)))
            (search-forward ":")
            (let ((begin-f (point)))
              (goto-char begin)
              (search-forward "{")
              (backward-char)
              (forward-sexp)
              (let ((x (make-overlay begin (point))))
                (overlay-put x 'face '(:box "yellow"))
                (run-at-time "0.01 second" nil
                             #'memetalk-remove-overlays (current-buffer)))
              (buffer-substring-no-properties begin-f (point)))))
          ((debug error) nil))))


(defun memetalk-get-begin-procedure-line ()
  (interactive)
  (with-current-buffer (current-buffer)
    (condition-case nil
        (save-excursion
          (re-search-backward
           "^\\s-*\\(init\\s-+\\|instance_method\\s-+\\|class_method\\s-+\\|\\)\\(\\w+\\): fun")
          (let ((current-line (line-number-at-pos (point))))
                current-line)))))
