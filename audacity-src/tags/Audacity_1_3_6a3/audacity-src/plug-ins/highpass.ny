;nyquist plug-in
;version 3
;type process
;categories "http://lv2plug.in/ns/lv2core#HighpassPlugin"
;name "High Pass Filter..."
;action "Performing High Pass Filter..."
;info "by Dominic Mazzoni, modified by David R. Sky\nReleased under terms of the GNU General Public License Version 2\nAttenuates frequencies below your specified cutoff frequency.\nHigher rolloff values give a sharper attenuation of frequencies below\nthe cutoff frequency. If using a rolloff of 6 dB, a [q] value greater than\ndefault 0.7 increases resonance ['ringing'] of the cutoff frequency and\ncould result in clipping."

;control rolloff-choice "     Rolloff [dB per octave]" choice " 3 dB, 6 dB,12 dB,18 dB,24 dB" 0
;control q "     Filter quality [q] for 6 dB rolloff" real "" 0.7071 .1 20
;control f "     Cutoff frequency [Hz]" real "" 1000 1 20000

; note that all Nyquist highpass functions 
; [hp, highpass2, highpass4, highpass6, highpass8]
; are defined below with -s suffix.
; This enables highpass functions other than hp 
; to deal with stereo selection,
; and dummy q arg for all but highpass2 

; 3dB/octave
(defun hp-s (s f q) ; dummy q arg
(hp s f))

; 6dB/octave
(defun highpass2-s (s f q)
(if (arrayp s)
(vector (highpass2 (aref s 0) f q)
(highpass2 (aref s 1) f q))
(highpass2 s f q)))

; 12dB/octave
(defun highpass4-s (s f q) ; dummy q arg
(if (arrayp s)
(vector (highpass4 (aref s 0) f)
(highpass4 (aref s 1) f))
(highpass4 s f)))

; 18dB/octave
(defun highpass6-s (s f q) ; dummy q arg
(if (arrayp s)
(vector (highpass6 (aref s 0) f)
(highpass6 (aref s 1) f))
(highpass6 s f)))

; 24dB/octave
(defun highpass8-s (s f q) ; dummy q arg
(if (arrayp s)
(vector (highpass8 (aref s 0) f)
(highpass8 (aref s 1) f))
(highpass8 s f)))

; check function: returns 1 on error
(defun check (arg min max)
(if (and (>= arg min) (<= arg max))
0 1))


; initialize blank error-msg
(setf error-msg "")

; check for erroneous q value
(setf error-msg (if 
(and (= rolloff-choice 1)
(= (check q 0.1 20) 1))
(strcat error-msg (format nil
"q value ~a lies outside valid range 0.1 to 20     
for your chosen rolloff of 6 dB per octave.
" q))
error-msg))

; check for erroneous frequency cutoff value
(setf error-msg (if 
(= (check f 1 20000) 0)
error-msg
(strcat error-msg (format nil
"Cutoff frequency ~a Hz lies outside valid range 1 to 20000.     
" f))))


(cond
((> (length error-msg) 0)
(setf error-msg (strcat (format nil
"Error - \n\nYou have entered at least one invalid value:
") error-msg))
(format nil "~a" error-msg)) 
;
(t ; perform highpass effect
(funcall (nth rolloff-choice '(hp-s highpass2-s highpass4-s highpass6-s highpass8-s)) 
s f q)))



; from previous commit
; arch-tag: 49302eba-9945-43d7-aade-f1c7eded27af

