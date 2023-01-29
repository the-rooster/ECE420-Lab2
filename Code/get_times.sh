declare -a n=(100 1000)

declare -a mains=(1 2 3 4)

mkdir results

for i in "${n[@]}"
do
 for main in "${mains[@]}"
 do
	 echo Running main${main} with string count ${i}
	 make main${main}
	 ./main${main} ${i} "127.0.0.1" 8080 2>&1 >/dev/null & PROC_ID=$!
	 ./test.sh ${i} "127.0.0.1" 8080
	 kill $PROC_ID
	 mv server_output_time_aggregated results/main${main}_times_n_${i}.txt
	 echo Done main${main}
 done
done
