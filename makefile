Player: player.o ffmpeg_decoder.o ffmpeg_resampler.o audio_player.o
	g++ player.o ffmpeg_decoder.o ffmpeg_resampler.o audio_player.o -o Player -lavformat -lavutil -lavcodec -lswresample -lpulse-simple -g

player.o: player.cpp ffmpeg_decoder.h ffmpeg_resampler.h audio_player.h
	g++ -c player.cpp -g

ffmpeg_decoder.o: ffmpeg_decoder.cpp ffmpeg_decoder.h
	g++ -c ffmpeg_decoder.cpp -g

ffmpeg_resampler.o: ffmpeg_resampler.cpp ffmpeg_resampler.h
	g++ -c ffmpeg_resampler.cpp -g

audio_player.o: audio_player.cpp audio_player.h
	g++ -c audio_player.cpp -g

clean:
	rm *.o
