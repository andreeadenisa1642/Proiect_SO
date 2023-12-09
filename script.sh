#!/bin/bash


#verifica argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <caracter>"
    exit 1
fi

caracter="$1" #argumentul
propozitii_corecte=0 #counter

while IFS= read -r line || [[ -n "$line" ]]; do
    if echo "$line" | grep -Eq "^[A-Z][a-zA-Z0-9 ,]*[\.\?\!]$"; then
        if ! echo "$line" | grep -Eq "(,|^)[ ]*si"; then
            if echo "$line" | grep -qi "$caracter"; then
                ((propozitii_corecte++))
            fi
        fi
    fi
done

echo "Propozitii corecte care contin carecaterul '$caracter': $propozitii_corecte"
