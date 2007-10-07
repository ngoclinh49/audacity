;nyquist plug-in
;version 3
;type generate
;name "Click Track..."
;action "Generating click track..."
;info "Written by Dominic Mazzoni, modified by David R. Sky\nReleased under terms of the GNU General Public License version 2\nGenerates a click track at the given tempo and beats per measure, using the\nclick sound type you choose below. To start the click track after time zero,\nenter the starting point in start-time-offset.\nTo create metronome-like click track, set beats-per-measure value to 1 or 2. \nPitches are set using MIDI numbers for example:\nC notes: 48, 60 [middle C], 72, 84, 96."

;control tempo "Tempo [beats per minute]" int "" 120 30 300
;control sig "Beats per measure [bar]" int "" 4 1 20
;control measures "Number of measures [bars]" int "" 16 1 1000
;control offset "Start time offset [seconds]" real "" 0 0 30
;control click-type "Click sound type" choice "ping,noise,tick" 0
;control q "Noise click resonance [q] [higher gives more defined pitch]" int "" 1 1 20
;control high "MIDI pitch of strong click" int "" 92 18 116
;control low "MIDI pitch of weak click" int "" 80 18 116

; original clicktrack.ny by Dominic Mazzoni,
; modified by David R. Sky September 2007
; original code kept 'as is'.
; now includes:
; choice between click sounds [ping {sinewave}, noise or tick],
; user-set MIDI pitch values for strong and weak clicks,
; resonance of noise clicks 
; [higher resonance gives noise clicks more discernable pitch],
; time offset for start of click track,
; and error-checking code to generate error message
; for such things as negative value inputs
; Drip sound generator by Paul Beach,
; used with permission.

(setf click-type (+ click-type 1))


; function to return error value of 0 
; if no error detected,
; otherwise returns 1 for later error message
(defun error-check (arg &optional (flonum 0))
(if (= flonum 0)
(if
(= arg (abs (truncate arg))) 0 1)
(if 
(= arg (abs arg)) 0 1)))


; input values error checks
; important note: for any future development,
; the following error check code must have the error check for 
; tempo be first
(if 
(and (= (error-check tempo) 0)
(> tempo 29) (< tempo 301))
(progn (setf error 0) (setf tempo-msg "          OK      - "))
(progn (setf error  1) (setf tempo-msg "=> => ERROR - ")))
(if (and (= (error-check sig) 0)
(> sig 0) (< sig 21))
(progn (setf error error) (setf sig-msg "          OK      - "))
(progn (setf error  1) (setf sig-msg "=> => ERROR - ")))
(if 
(and (= (error-check measures ) 0)
(> measures 0) (< measures 1001))
(progn (setf error error) (setf measures-msg "          OK      - "))
(progn (setf error  1) (setf measures-msg "=> => ERROR - ")))
(if 
(and (= (error-check offset 1) 0)
(<= offset 30.0))
(progn (setf error error) (setf offset-msg "          OK      - "))
(progn (setf error  1) (setf offset-msg "=> => ERROR - ")))
(if
(and (= (error-check q) 0)
(> q 0))
(progn (setf error error) (setf q-msg "          OK      - "))
(progn (setf error  1) (setf q-msg "=> => ERROR - ")))
(if
(and (>= high 18) (<= high 116))
(progn (setf error error) (setf high-msg "          OK        - "))
(progn (setf error  1) (setf high-msg "=> => ERROR - ")))
(if
(and (>= low 18) (<= low 116))
(progn (setf error error) (setf low-msg "          OK      - "))
(progn (setf error  1) (setf low-msg "=> => ERROR - ")))

(cond
; if error value is 1, give error message
((= error 1) 
(format nil "Error - ~%
One or more of your input values is incorrect.\nValid input values are listed above the values you\nentered, and each one shown as ERROR or OK ~%
Tempo - positive integer 30 to 300\n~a ~a beats per minute
Beats per measure - positive value 1 to 20\n~a ~a beats per measure [bar]
Number of measures [bars] - positive integer 1 to 1000\n~a ~a measures [bars]
Time offset - 30.0 seconds or lower\n~a ~a seconds
Noise click resonance [q] - value above 0\n~a ~a
MIDI pitches of clicks - values between 18 and 116\n~a ~a high MIDI pitch
~a ~a low MIDI pitch ~%"
tempo-msg tempo sig-msg sig measures-msg measures offset-msg offset 
q-msg q high-msg high low-msg low ))

; no error so generate click track
(t
(setf ticklen 0.01) ; duration of 1 click
(setf beatlen (/ 60.0 tempo))


; function to generate drip sound clicks
; code by Paul Beach www.proviewlandscape.com/liss/
; stretch-abs function makes this sound more like 'tick' sounds
(defun drip (p) ; p is pitch in hz
(lp 
(stretch 1
(mult (exp-dec 0 0.015 0.25) 
( sim
(mult (hzosc (*  2.40483  p))  0.5 )
(mult (hzosc (*  5.52008  p))  0.25 )
(mult (hzosc (* 8.653  p))  0.125 )
(mult (hzosc (* 11.8  p))  0.0625 )
)
)
) 
440))


; function used to normalize noise and tick clicks
; this function is necessary because filtering 
; changes amplitude of filtered noise clicks
(defun normalize (sound)
(setf peak-level (peak sound ny:all))
(scale (/ 1.0 peak-level) sound))


; make one measure
(setf measure (stretch-abs ticklen (mult 0.75 
; pwl is used to add fast [5ms] fade-in and fade-out of clicks
(pwl 0 0 0.005 1 0.995 1 1 0 1)
(cond
((= click-type 1) ; ping accented clicks
(osc high))
((= click-type 2) ; noise accented clicks
(normalize (lowpass2 (noise 1) (step-to-hz high) q)))
((= click-type 3) ; tick accented clicks
(normalize (drip (step-to-hz high)))) ))))
(dotimes (x (- sig 1))
  (setf measure (sim measure
                     (at (* beatlen (+ x 1))                 
                         (stretch-abs ticklen (mult 0.5 
; again, pwl adds 5ms fade-in and fade-out to clicks
(pwl 0 0 0.005 1 0.995 1 1 0 1)
(cond
((= click-type 1) ;ping tone unaccented clicks
(osc low))
((= click-type 2) ; noise unaccented clicks
(normalize (lowpass2 (noise 1) (step-to-hz low) q)))
((= click-type 3) ; tick unaccented clicks
(normalize (drip (step-to-hz low)))) )))))))
; make the measure exactly the right length
(setf measure (sim measure
                   (stretch-abs (* sig beatlen) (const 0.0))))

; loop measure n [measures-1] times
(setf result measure)
(dotimes (x (- measures 1))
  (setf result (seq result measure)))
; add time offset to result,
; if offset > 0 seconds
(setf result (if (= offset 0) result
(sim (s-rest offset) (at-abs offset (cue result)))))

; return [click track] result
result

) ; end t
) ; end cond

; arch-tag: 73fbc0e9-548b-4143-b8ac-13437b9154a7


