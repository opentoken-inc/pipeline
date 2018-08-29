"----------------------------
" Recommended vim settings

" Allow switching between unsaved buffers
set hidden

" Better completion
set wildmenu
set wildmode=list:longest

" Show partial commands in the last line of the screen
set showcmd

" Highlight searches
set hlsearch

"----------------------------
" Encouraged/Usability settings


set ruler
set confirm

set backspace=indent,eol,start


" Fast scrolling
set ttyfast
set lazyredraw


" Relative numbers, with current line number at cursor
set relativenumber
set number

" undo between instances
set undofile

if has("autocmd")
	filetype plugin indent on
endif

" Indentation settings
set expandtab " Make sure that every file uses real tabs, not spaces
set shiftround  " Round indent to multiple of 'shiftwidth'

" Set the tab width
let s:tabwidth=2
exec 'set tabstop='    .s:tabwidth
exec 'set shiftwidth=' .s:tabwidth
exec 'set softtabstop='.s:tabwidth

syntax on
syntax enable
syntax sync minlines=200
set ffs=unix
set ff=unix
set splitbelow
set splitright
set textwidth=80


" Searching/Moving
set mouse=nv

" Resizing in tmux
if exists('$TMUX') && !has("nvim")
  if has("mouse_sgr")
    set ttymouse=sgr
  else
    set ttymouse=xterm2
  end
endif

set gdefault
set incsearch
set showmatch
set hlsearch

set ignorecase
set smartcase

" Settings from vim-sensible
set complete-=i
set smarttab
set nrformats-=octal
set ttimeout
set ttimeoutlen=100
set laststatus=2
if !empty(&viminfo)
  set viminfo^=!
endif
set sessionoptions-=options

" Colors if has('gui_running') set guioptions-=T " no toolbar set guifont=Consolas endif " Allow color schemes to do bright colors without forcing bold.  if &t_Co == 8 && $TERM !~# '^linux\|^Eterm' set t_Co=16 endif 
set background=dark
let g:solarized_italic = 1
colorscheme solarized

" Comments should be in italics
highlight Comment gui=italic 

set completeopt-=preview

let g:tex_indent_items=0
let g:tex_flavor='latex'

"" Various Personal Remappings
let mapleader = ","

" I'll launch with "vim -E" if I want Ex mode
nnoremap Q <nop>

" Space inserts a space
nmap <Space> i <Esc>r
"Ctrl-c closes buffer but not window
nnoremap <C-c> :bp\|bd # <CR>

"Ctrl-x closes window
nnoremap <C-x> :q <CR>
