<template>
  <router-view />
</template>

<script>
import { defineComponent } from 'vue'
import { useQuasar } from 'quasar'
import "animate.css"
export default defineComponent({
  name: 'App',
  setup() {
    const $q = useQuasar()
    $q.dark.set(true)
  }
})
</script>

<style>
body::-webkit-scrollbar {
  display: none;
}

body {
  -ms-overflow-style: none;
  scrollbar-width: none;
}
</style>