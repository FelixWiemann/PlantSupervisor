FROM node:lts
ENV NODE_ENV=production
ENV TERM=xterm
WORKDIR /usr/src/app
COPY ["package.json", "package-lock.json*", "npm-shrinkwrap.json*", "./"]
RUN npm install --production --silent
COPY . .
EXPOSE 4116
RUN chown -R node /usr/src/app
RUN chmod +X ./listen.sh
USER node

USER root 
RUN apt-get update -y; exit 0
RUN apt-get install tcpdump -y
RUN apt-get install net-tools wireless-tools -y 
RUN ifconfig
#RUN ifconfig wlan0 down
#RUN ifconfig wlan0 mode monitor
#RUN ifconfig wlan0 up
#RUN ifconfig

RUN make
#
# CMD ["ip", "addr", "show", "wlan0"]
# CMD [ "sh" ]

# CMD ["tcpdump", "-i any"]
#CMD ["ifconfig"]
CMD ["./bin/exec", "wlan0"]
# CMD ["sh","./listen.sh"]